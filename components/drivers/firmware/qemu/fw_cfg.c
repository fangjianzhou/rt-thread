/*
 * Copyright (c) 2006-2023, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2023-02-25     GuEe-GUI     the first version
 */

#include <rthw.h>
#include <rtthread.h>
#include <rtdevice.h>
#include <rtservice.h>

#define DBG_TAG "fw.qemu"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

#include <mmu.h>
#include <cpuport.h>

#include <drivers/ofw.h>
//#include <dfs_directfs.h>

#include "fw_cfg.h"

/* arch-specific ctrl & data register offsets are not available in ACPI, DT */
#if !(defined(FW_CFG_CTRL_OFF) && defined(FW_CFG_DATA_OFF))
# if (defined(ARCH_ARM) || defined(ARCH_ARMV8) || defined(ARCH_LOONGARCH))
#  define FW_CFG_CTRL_OFF 0x08
#  define FW_CFG_DATA_OFF 0x00
#  define FW_CFG_DMA_OFF 0x10
# elif defined(ARCH_PARISC) /* parisc */
#  define FW_CFG_CTRL_OFF 0x00
#  define FW_CFG_DATA_OFF 0x04
# elif (defined(ARCH_PPC) || defined(ARCH_SPARC32))     /* ppc/mac,sun4m */
#  define FW_CFG_CTRL_OFF 0x00
#  define FW_CFG_DATA_OFF 0x02
# elif (defined(ARCH_IA32) || defined(ARCH_SPARC64))    /* x86, sun4u */
#  define FW_CFG_CTRL_OFF 0x00
#  define FW_CFG_DATA_OFF 0x01
#  define FW_CFG_DMA_OFF 0x04
# else
#  error "QEMU FW_CFG not available on this architecture!"
# endif
#endif

struct fw_cfg_info
{
    struct rt_object obj;

    rt_list_t list;

    rt_uint32_t size;
    rt_uint16_t select;
    char name[FW_CFG_MAX_FILE_PATH];
};
#define raw_to_fw_cfg_info(raw) rt_container_of(raw, struct fw_cfg_info, obj)

static void *_fw_cfg_dev_base;
static void *_fw_cfg_reg_ctrl;
static void *_fw_cfg_reg_data;
static void *_fw_cfg_reg_dma;
static rt_bool_t _fw_cfg_is_mmio;
static rt_uint32_t _fw_cfg_rev = 0;

static rt_object_t _directfs_firmware_root = RT_NULL;
static struct rt_spinlock _fw_cfg_dev_lock = { 0 };
static rt_list_t _fw_cfg_nodes = RT_LIST_OBJECT_INIT(_fw_cfg_nodes);

static void fw_cfg_sel_endianness(rt_uint16_t key)
{
    rt_hw_barrier(dsb, st);

    if (_fw_cfg_is_mmio)
    {
        HWREG16(_fw_cfg_reg_ctrl) = rt_cpu_to_be16(key);
    }
    else
    {
        HWREG16(_fw_cfg_reg_ctrl) = key;
    }
}

static rt_base_t fw_cfg_read_blob(rt_uint16_t key, void *buf, rt_off_t pos, rt_size_t count)
{
    rt_uint8_t tmp;

    rt_spin_lock(&_fw_cfg_dev_lock);
    fw_cfg_sel_endianness(key);

    while (pos-- > 0)
    {
        tmp = HWREG8(_fw_cfg_reg_data);
    }

    if (count)
    {
        int loop = count;
        rt_uint8_t *buffer = buf;

        do {
            tmp = HWREG8(_fw_cfg_reg_data);
            *buffer++ = tmp;
        } while (--loop);
    }

    rt_spin_unlock(&_fw_cfg_dev_lock);

    return count;
}

rt_inline rt_bool_t fw_cfg_dma_enabled(void)
{
    return (_fw_cfg_rev & FW_CFG_VERSION_DMA) && _fw_cfg_reg_dma;
}

/* qemu fw_cfg device is sync today, but spec says it may become async */
static void fw_cfg_wait_for_control(struct fw_cfg_dma_access *dma)
{
    for (;;)
    {
        rt_uint32_t ctrl = rt_be32_to_cpu(HWREG32(&dma->control));

        /* do not reorder the read to d->control */
        rt_hw_barrier(dsb, ld);

        if ((ctrl & ~FW_CFG_DMA_CTL_ERROR) == 0)
        {
            break;
        }

        rt_hw_cpu_relax();
    }
}

static rt_base_t fw_cfg_dma_transfer(void *address, rt_uint32_t length, rt_uint32_t control)
{
    rt_ubase_t dma_pa;
    rt_base_t res = length;
    struct fw_cfg_dma_access dma =
    {
        .address = rt_cpu_to_be64((rt_uint64_t)(address ? rt_kmem_v2p(address) : 0)),
        .length = rt_cpu_to_be32(length),
        .control = rt_cpu_to_be32(control),
    };

    dma_pa = (rt_ubase_t)rt_kmem_v2p(&dma);

    HWREG32(_fw_cfg_reg_dma) = rt_cpu_to_be32((rt_uint64_t)dma_pa >> 32);
    /* force memory to sync before notifying device via MMIO */
    rt_hw_barrier(dsb, st);
    HWREG32(_fw_cfg_reg_dma + 4) = rt_cpu_to_be32(dma_pa);

    fw_cfg_wait_for_control(&dma);

    if ((rt_be32_to_cpu(HWREG32(&dma.control)) & FW_CFG_DMA_CTL_ERROR))
    {
        res = -RT_EIO;
    }

    return res;
}

static rt_base_t fw_cfg_write_blob(rt_uint16_t key, void *buf, rt_off_t pos, rt_size_t count)
{
    rt_base_t res = count;

    rt_spin_lock(&_fw_cfg_dev_lock);

    if (pos == 0)
    {
        res = fw_cfg_dma_transfer(buf, count, key << 16 | FW_CFG_DMA_CTL_SELECT | FW_CFG_DMA_CTL_WRITE);
    }
    else
    {
        fw_cfg_sel_endianness(key);
        res = fw_cfg_dma_transfer(RT_NULL, pos, FW_CFG_DMA_CTL_SKIP);

        if (res >= 0)
        {
            res = fw_cfg_dma_transfer(buf, count, FW_CFG_DMA_CTL_WRITE);
        }
    }

    rt_spin_unlock(&_fw_cfg_dev_lock);

    return res;
}

#ifdef RT_USING_CRASH_CORE
static rt_base_t fw_cfg_write_vmcoreinfo(const struct fw_cfg_file *file)
{
    rt_ubase_t res;
    struct fw_cfg_vmcoreinfo info =
    {
        .guest_format = rt_cpu_to_le16(FW_CFG_VMCOREINFO_FORMAT_ELF),
        .size = rt_cpu_to_le32(VMCOREINFO_NOTE_SIZE),
        .paddr = rt_cpu_to_le64(vmcoreinfo_note_paddr()),
    };

    res = fw_cfg_write_blob(rt_be16_to_cpu(file->select), &info, 0, sizeof(struct fw_cfg_vmcoreinfo));

    return res;
}
#endif /* RT_USING_CRASH_CORE */

#define fourcc_code(a, b, c, d) ((rt_uint32_t)(a) | ((rt_uint32_t)(b) << 8) | ((rt_uint32_t)(c) << 16) | ((rt_uint32_t)(d) << 24))

#define DRM_FORMAT_RGB565   fourcc_code('R', 'G', '1', '6')
#define DRM_FORMAT_RGB888   fourcc_code('R', 'G', '2', '4')
#define DRM_FORMAT_XRGB8888 fourcc_code('X', 'R', '2', '4')

static rt_base_t fw_cfg_setup_ramfb(const struct fw_cfg_file *file)
{
    rt_ubase_t res;
    void *fb = rt_malloc_align(800 * 600 * 4, 4096);
    struct fw_cfg_ram_fb ram_fb =
    {
        .addr   = rt_cpu_to_be64((rt_ubase_t)rt_kmem_v2p(fb)),
        .fourcc = rt_cpu_to_be32(DRM_FORMAT_XRGB8888),
        .flags  = rt_cpu_to_be32(0),
        .width  = rt_cpu_to_be32(800),
        .height = rt_cpu_to_be32(600),
        .stride = rt_cpu_to_be32(4 * 800),
    };

    res = fw_cfg_write_blob(rt_be16_to_cpu(file->select), &ram_fb, 0, sizeof(struct fw_cfg_ram_fb));

    rt_memset(fb, 0xff, 800 * 600 * 4);

    return res;
}

static rt_ssize_t fw_cfg_directfs_read_raw(rt_object_t obj, struct directfs_bin_attribute *attr,
        char *buffer, rt_off_t pos, rt_size_t count)
{
    rt_ssize_t res;
    struct fw_cfg_info *info = raw_to_fw_cfg_info(obj);

    if (pos <= info->size)
    {
        if (count > info->size - pos)
        {
            count = info->size - pos;
        }

        res = fw_cfg_read_blob(info->select, buffer, pos, count);
    }
    else
    {
        res = -RT_EINVAL;
    }

    return res;
}

static struct directfs_bin_attribute fw_cfg_directfs_attr_raw =
{
    .attr =
    {
        .name = "raw",
    },
    .read = fw_cfg_directfs_read_raw,
};

static rt_err_t fw_cfg_register_file(const struct fw_cfg_file *file)
{
    rt_err_t res = RT_EOK;
    struct fw_cfg_info *info = rt_malloc(sizeof(*info));

    if (info)
    {
        char *path;
        rt_object_t parent = _directfs_firmware_root;

    #ifdef RT_USING_CRASH_CORE
        if (fw_cfg_dma_enabled() && !rt_strcmp(file->name, FW_CFG_VMCOREINFO_FILENAME))
        {
            if (fw_cfg_write_vmcoreinfo(file) < 0)
            {
                LOG_W("failed to write vmcoreinfo");
            }
        }
    #endif /* RT_USING_CRASH_CORE */

        if (fw_cfg_dma_enabled() && !rt_strcmp(file->name, FW_CFG_RAMFB_FILENAME))
        {
            if (fw_cfg_setup_ramfb(file) < 0)
            {
                LOG_W("failed to setup ramfb");
            }
        }

        rt_list_init(&info->list);

        info->size = rt_be32_to_cpu(file->size);
        info->select = rt_be16_to_cpu(file->select);
        rt_strncpy(info->name, file->name, FW_CFG_MAX_FILE_PATH);

        rt_list_insert_before(&_fw_cfg_nodes, &info->list);

        path = info->name;

        while (*path)
        {
            const char *basename = path;

            while (*path && *path != '/')
            {
                ++path;
            }

            if (*path)
            {
                rt_object_t dir;

                *path = '\0';
                dir = dfs_directfs_find_object(parent, basename);

                if (!dir)
                {
                    dir = rt_malloc(sizeof(*dir));

                    if (!dir)
                    {
                        break;
                    }

                    dfs_directfs_create_link(parent, dir, basename);
                }

                parent = dir;
                *path = '/';
            }
            else
            {
                dfs_directfs_create_link(parent, &info->obj, basename);
                dfs_directfs_create_bin_file(&info->obj, &fw_cfg_directfs_attr_raw);

                break;
            }

            ++path;
        }
    }

    return res;
}

static rt_err_t fw_cfg_register_dir_entries(void)
{
    rt_err_t err = 0;
    rt_uint32_t count;
    rt_size_t dir_size;
    rt_be32_t files_count;
    struct fw_cfg_file *dir;

    err = fw_cfg_read_blob(FW_CFG_FILE_DIR, &files_count, 0, sizeof(files_count));

    if (err >= 0)
    {
        count = rt_be32_to_cpu(files_count);
        dir_size = count * sizeof(struct fw_cfg_file);

        dir = rt_malloc(dir_size);

        if (dir)
        {
            err = fw_cfg_read_blob(FW_CFG_FILE_DIR, dir, sizeof(files_count), dir_size);

            if (err >= 0)
            {
                _directfs_firmware_root = dfs_directfs_find_object(RT_NULL, "firmware");

                for (int i = 0; i < count; ++i)
                {
                    if ((err = fw_cfg_register_file(&dir[i])))
                    {
                        break;
                    }
                }
            }

            rt_free(dir);
        }
        else
        {
            err = -RT_ENOMEM;
        }
    }

    return err;
}

static rt_err_t qemu_fw_cfg_ofw_init(struct rt_platform_device *pdev, rt_uint32_t *ctrl, rt_uint32_t *data, rt_uint32_t *dma)
{
    struct rt_ofw_node *np = pdev->parent.ofw_node;

    rt_ofw_prop_read_u32(np, "ctrl", ctrl);
    rt_ofw_prop_read_u32(np, "data", data);
    rt_ofw_prop_read_u32(np, "dma", dma);

    _fw_cfg_dev_base = rt_ofw_iomap(np, 0);

    return _fw_cfg_dev_base ? RT_EOK : -RT_ERROR;
}

static rt_err_t qemu_fw_cfg_probe(struct rt_platform_device *pdev)
{
    rt_le32_t rev;
    rt_err_t err = RT_EOK;
    char sig[FW_CFG_SIG_SIZE];
    rt_uint32_t ctrl = FW_CFG_CTRL_OFF, data = FW_CFG_DATA_OFF, dma = FW_CFG_DMA_OFF;

    if ((err = qemu_fw_cfg_ofw_init(pdev, &ctrl, &data, &dma)))
    {
        goto _fail;
    }

#ifdef ARCH_SUPPORT_PIO
    _fw_cfg_is_mmio = RT_FALSE;
#else
    _fw_cfg_is_mmio = RT_TRUE;
#endif

    _fw_cfg_reg_ctrl = _fw_cfg_dev_base + ctrl;
    _fw_cfg_reg_data = _fw_cfg_dev_base + data;
    _fw_cfg_reg_dma = _fw_cfg_dev_base + dma;

    if (fw_cfg_read_blob(FW_CFG_SIGNATURE, sig, 0, FW_CFG_SIG_SIZE) < 0 || rt_memcmp(sig, "QEMU", FW_CFG_SIG_SIZE))
    {
        err = -RT_ENOSYS;
        goto _fail;
    }

    if (fw_cfg_read_blob(FW_CFG_ID, &rev, 0, sizeof(rev)) < 0)
    {
        err = -RT_ENOSYS;
        goto _fail;
    }

    _fw_cfg_rev = rt_le32_to_cpu(rev);

    fw_cfg_register_dir_entries();

_fail:
    return err;
}

static const struct rt_ofw_node_id qemu_fw_cfg_ofw_ids[] =
{
    { .compatible = "qemu,fw-cfg-mmio", },
    { /* sentinel */ }
};

static struct rt_platform_driver qemu_fw_cfg_driver =
{
    .name = "qemu-fw-cfg",
    .ids = qemu_fw_cfg_ofw_ids,

    .probe = qemu_fw_cfg_probe,
};

static int qemu_fw_cfg_drv_register(void)
{
    rt_platform_driver_register(&qemu_fw_cfg_driver);

    return 0;
}
INIT_SUBSYS_EXPORT(qemu_fw_cfg_drv_register);
