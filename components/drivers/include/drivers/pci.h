/*
 * Copyright (c) 2006-2022, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-08-25     GuEe-GUI     first version
 */

#ifndef __PCI_H__
#define __PCI_H__

#include <bitmap.h>
#include <ioremap.h>
#include <drivers/ofw.h>
#include <drivers/core/rtdm.h>
#include <drivers/core/device.h>
#include <drivers/core/driver.h>

#include "../../pci/pci_ids.h"
#include "../../pci/pci_regs.h"

#define RT_PCI_INTX_PIN_MAX         4
#define RT_PCI_BAR_NR_MAX           6
#define RT_PCI_DEVICE_MAX           32
#define RT_PCI_FUNCTION_MAX         8

#define RT_PCI_FIND_CAP_TTL         48

/*
 * The PCI interface treats multi-function devices as independent
 * devices.  The slot/function address of each device is encoded
 * in a single byte as follows:
 *
 *  7:3 = slot
 *  2:0 = function
 */
#define RT_PCI_DEVFN(slot, func)    ((((slot) & 0x1f) << 3) | ((func) & 0x07))
#define RT_PCI_SLOT(devfn)          (((devfn) >> 3) & 0x1f)
#define RT_PCI_FUNC(devfn)          ((devfn) & 0x07)

struct rt_pci_bus_region
{
    rt_uint64_t phy_addr;
    rt_uint64_t cpu_addr;
    rt_uint64_t size;

    rt_uint64_t bus_start;

#define PCI_BUS_REGION_F_NONE       0xffffffff    /* PCI no memory */
#define PCI_BUS_REGION_F_MEM        0x00000000    /* PCI memory space */
#define PCI_BUS_REGION_F_IO         0x00000001    /* PCI IO space */
#define PCI_BUS_REGION_F_PREFETCH   0x00000008    /* Prt_refetchable PCI memory */
    rt_ubase_t flags;
};

struct rt_pci_bus_resource
{
    rt_ubase_t base;
    rt_size_t size;

    rt_ubase_t flags;
};

enum
{
    PCI_F_MULTI_FUNCTION,           /* Multi-function device */
    PCI_F_NO_64BIT_MSI,             /* May only use 32-bit MSIs */
    PCI_F_NO_MSI,                   /* May not use MSI */
    PCI_F_ARI,                      /* ARI forwarding */
    PCI_F_MSI,                      /* MSI enable */
    PCI_F_MSIX,                     /* MSIx enable */
    PCI_F_BROKEN_INTX_MASKING,      /* INTx masking can't be used */

    PCI_FLAGS_NR
};

/*
 * PCI topology:
 *
 *   +-----+-----+         +-------------+   PCI Bus 0  +------------+ PCI Bus 1
 *   | RAM | CPU |---------| Host Bridge |--------+-----| PCI Bridge |-----+
 *   +-----+-----+         +-------------+        |     +------------+     |    +-------------+
 *                                                |                        +----| End Point 2 |
 *  +-------------+         +-------------+       |     +-------------+    |    +-------------+
 *  | End Point 5 |----+    | End Point 0 |-------+     | End Point 3 |----+
 *  +-------------+    |    +-------------+       |     +-------------+    |
 *                     |                          |                        |
 *  +-------------+    |    +-------------+       |     +-------------+    |    +-------------+
 *  | End Point 6 |----+----|  ISA Bridge |-------+-----| End Point 1 |    +----| End Point 4 |
 *  +-------------+         +-------------+       |     +-------------+         +-------------+
 *                                                |
 *         +------+         +----------------+    |
 *         | Port |---------| CardBus Bridge |----+
 *         +------+         +----------------+
 */

struct rt_pci_bus;

struct rt_pci_device_id
{
#define PCI_ANY_ID   (~0)
#define RT_PCI_DEVICE_ID(vend, dev)         \
    .vendor = (vend),                       \
    .device = (dev),                        \
    .subsystem_vendor = PCI_ANY_ID,  \
    .subsystem_device = PCI_ANY_ID

    rt_uint32_t vendor, device;     /* Vendor and device ID or PCI_ANY_ID */
    rt_uint32_t subsystem_vendor;   /* Subsystem ID's or PCI_ANY_ID */
    rt_uint32_t subsystem_device;   /* Subsystem ID's or PCI_ANY_ID */
    rt_uint32_t class, class_mask;  /* (class, subclass, prog-if) triplet */

    void *data;
};

struct rt_pci_device
{
    struct rt_device parent;
    const char *name;

    rt_list_t list;
    rt_list_t list_in_bus;
    struct rt_pci_bus *bus;
    struct rt_pci_bus *subbus;      /* In PCI-to-PCI bridge, 'End Point' or 'Port' is NULL */

    const struct rt_pci_device_id *id;

    rt_uint32_t devfn;              /* Encoded device & function index */
    rt_uint16_t vendor;
    rt_uint16_t device;
    rt_uint16_t subsystem_vendor;
    rt_uint16_t subsystem_device;
    rt_uint32_t class;              /* 3 bytes: (base, sub, prog-if) */
    rt_uint8_t revision;
    rt_uint8_t hdr_type;
    rt_uint8_t max_latency;
    rt_uint8_t min_grantl;
    rt_uint8_t int_pin;
    rt_uint8_t int_line;

    void *sysdata;

    int irq;
    rt_uint8_t pin;

    struct rt_pci_bus_resource resource[RT_PCI_BAR_NR_MAX];

    rt_uint8_t pcie_cap;
    rt_uint8_t msi_cap;
    rt_uint8_t msix_cap;
    RT_DECLARE_BITMAP(flags, PCI_FLAGS_NR);

#ifdef RT_PCI_MSI
    void *msix_base;
    rt_list_t msi_desc_nodes;
    struct rt_spinlock msi_lock;
#endif
};

struct rt_pci_host_bridge
{
    struct rt_device parent;

    rt_uint32_t busnr;
    rt_uint32_t domain;

    struct rt_pci_bus *root_bus;
    struct rt_pci_ops *ops;

    rt_uint32_t bus_range[2];
    rt_size_t bus_regions_nr;
    struct rt_pci_bus_region *bus_regions;

    rt_uint8_t (*irq_slot)(struct rt_pci_device *pdev, rt_uint8_t *pinp);
    int (*irq_map)(struct rt_pci_device *pdev, rt_uint8_t slot, rt_uint8_t pin);

    void *sysdata;
    rt_uint8_t priv[0];
};
#define rt_device_to_pci_host_bridge(dev) rt_container_of(dev, struct rt_pci_host_bridge, parent)

struct rt_pci_ops
{
    rt_err_t (*add)(struct rt_pci_bus *bus);
    rt_err_t (*remove)(struct rt_pci_bus *bus);

    void *(*map)(struct rt_pci_bus *bus, rt_uint32_t devfn, int reg);

    rt_err_t (*read)(struct rt_pci_bus *bus, rt_uint32_t devfn, int reg, int width, rt_uint32_t *value);
    rt_err_t (*write)(struct rt_pci_bus *bus, rt_uint32_t devfn, int reg, int width, rt_uint32_t value);
};

struct rt_pci_bus
{
    rt_list_t list;
    rt_list_t children_nodes;
    rt_list_t devices_nodes;
    struct rt_pci_bus *parent;

    union
    {
        /* In PCI-to-PCI bridge, parent is not NULL */
        struct rt_pci_device *self;
        /* In Host bridge, this is Root bus ('PCI Bus 0') */
        struct rt_pci_host_bridge *host_bridge;
    };

    struct rt_pci_ops *ops;

    char name[48];
    char number;

    void *sysdata;
};

struct rt_pci_driver
{
    struct rt_driver parent;

    const char *name;
    const struct rt_pci_device_id *ids;

    rt_err_t (*probe)(struct rt_pci_device *pdev);
};

struct rt_pci_msix_entry
{
    rt_uint32_t vector; /* Kernel uses to write allocated vector */
    rt_uint16_t entry;  /* Driver uses to specify entry, OS writes */
};

void rt_pci_msi_init(struct rt_pci_device *pdev);
void rt_pci_msix_init(struct rt_pci_device *pdev);

rt_inline rt_bool_t rt_pci_device_test_flag(const struct rt_pci_device *pdev, int flag)
{
    return rt_bitmap_test_bit((rt_bitmap_t *)pdev->flags, flag);
}

rt_inline void rt_pci_device_set_flag(struct rt_pci_device *pdev, int flag)
{
    rt_bitmap_set_bit(pdev->flags, flag);
}

rt_inline rt_bool_t rt_pci_device_test_and_set_flag(struct rt_pci_device *pdev, int flag)
{
    rt_bool_t res = rt_pci_device_test_flag(pdev, flag);

    rt_pci_device_set_flag(pdev, flag);

    return res;
}

rt_inline void rt_pci_device_clear_flag(struct rt_pci_device *pdev, int flag)
{
    rt_bitmap_clear_bit(pdev->flags, flag);
}

struct rt_pci_host_bridge *rt_pci_host_bridge_alloc(rt_size_t priv_size);
rt_err_t rt_pci_host_bridge_init(struct rt_pci_host_bridge *host_bridge);
rt_err_t rt_pci_host_bridge_probe(struct rt_pci_host_bridge *host_bridge);

struct rt_pci_device *rt_pci_alloc_device(struct rt_pci_bus *bus);
struct rt_pci_device *rt_pci_scan_single_device(struct rt_pci_bus *bus, rt_uint32_t devfn);
rt_err_t rt_pci_setup_device(struct rt_pci_device *pdev);
rt_size_t rt_pci_scan_slot(struct rt_pci_bus *bus, rt_uint32_t devfn);
rt_uint32_t rt_pci_scan_child_buses(struct rt_pci_bus *bus, rt_size_t buses);

rt_err_t rt_pci_host_bridge_register(struct rt_pci_host_bridge *host_bridge);
rt_err_t rt_pci_scan_root_bus_bridge(struct rt_pci_host_bridge *host_bridge);

rt_uint32_t rt_pci_domain(struct rt_pci_device *pdev);

rt_uint8_t rt_pci_bus_find_capability(struct rt_pci_bus *bus, rt_uint32_t devfn, int cap);
rt_uint8_t rt_pci_find_capability(struct rt_pci_device *pdev, int cap);
rt_uint8_t rt_pci_find_next_capability(struct rt_pci_device *pdev, rt_uint8_t pos, int cap);

struct rt_pci_bus *rt_pci_find_root_bus(struct rt_pci_bus *bus);
struct rt_pci_host_bridge *rt_pci_find_host_bridge(struct rt_pci_bus *bus);

rt_inline rt_bool_t rt_pci_is_root_bus(struct rt_pci_bus *bus)
{
    return bus->parent ? RT_FALSE : RT_TRUE;
}

rt_inline rt_bool_t rt_pci_is_bridge(struct rt_pci_device *pdev)
{
    return pdev->hdr_type == PCIM_HDRTYPE_BRIDGE ||
            pdev->hdr_type == PCIM_HDRTYPE_CARDBUS;
}

#define rt_pci_foreach_bridge(pdev, bus) \
    rt_list_for_each_entry(pdev, &bus->devices_nodes, list_in_bus) \
        if (rt_pci_is_bridge(pdev))

rt_err_t rt_pci_bus_read_config_u8(struct rt_pci_bus *bus, rt_uint32_t devfn, int pos, rt_uint8_t *value);
rt_err_t rt_pci_bus_read_config_u16(struct rt_pci_bus *bus, rt_uint32_t devfn, int pos, rt_uint16_t *value);
rt_err_t rt_pci_bus_read_config_u32(struct rt_pci_bus *bus, rt_uint32_t devfn, int pos, rt_uint32_t *value);

rt_err_t rt_pci_bus_write_config_u8(struct rt_pci_bus *bus, rt_uint32_t devfn, int reg, rt_uint8_t value);
rt_err_t rt_pci_bus_write_config_u16(struct rt_pci_bus *bus, rt_uint32_t devfn, int reg, rt_uint16_t value);
rt_err_t rt_pci_bus_write_config_u32(struct rt_pci_bus *bus, rt_uint32_t devfn, int reg, rt_uint32_t value);

rt_err_t rt_pci_bus_read_config_uxx(struct rt_pci_bus *bus, rt_uint32_t devfn, int reg, int width, rt_uint32_t *value);
rt_err_t rt_pci_bus_write_config_uxx(struct rt_pci_bus *bus, rt_uint32_t devfn, int reg, int width, rt_uint32_t value);

rt_inline rt_err_t rt_pci_read_config_u8(const struct rt_pci_device *pdev, int reg, rt_uint8_t *value)
{
    return rt_pci_bus_read_config_u8(pdev->bus, pdev->devfn, reg, value);
}

rt_inline rt_err_t rt_pci_read_config_u16(const struct rt_pci_device *pdev, int reg, rt_uint16_t *value)
{
    return rt_pci_bus_read_config_u16(pdev->bus, pdev->devfn, reg, value);
}

rt_inline rt_err_t rt_pci_read_config_u32(const struct rt_pci_device *pdev, int reg, rt_uint32_t *value)
{
    return rt_pci_bus_read_config_u32(pdev->bus, pdev->devfn, reg, value);
}

rt_inline rt_err_t rt_pci_write_config_u8(const struct rt_pci_device *pdev, int reg, rt_uint8_t value)
{
    return rt_pci_bus_write_config_u8(pdev->bus, pdev->devfn, reg, value);
}

rt_inline rt_err_t rt_pci_write_config_u16(const struct rt_pci_device *pdev, int reg, rt_uint16_t value)
{
    return rt_pci_bus_write_config_u16(pdev->bus, pdev->devfn, reg, value);
}

rt_inline rt_err_t rt_pci_write_config_u32(const struct rt_pci_device *pdev, int reg, rt_uint32_t value)
{
    return rt_pci_bus_write_config_u32(pdev->bus, pdev->devfn, reg, value);
}

rt_inline rt_bool_t rt_pci_enabled(struct rt_pci_device *pdev, int bit)
{
    return rt_bitmap_test_bit(pdev->flags, bit);
}

#ifdef RT_USING_OFW
int rt_pci_ofw_irq_parse_and_map(struct rt_pci_device *pdev, rt_uint8_t slot, rt_uint8_t pin);
rt_err_t rt_pci_ofw_parse_ranges(struct rt_ofw_node *dev_np, struct rt_pci_host_bridge *host_bridge);
rt_err_t rt_pci_ofw_host_bridge_init(struct rt_ofw_node *dev_np, struct rt_pci_host_bridge *host_bridge);
rt_err_t rt_pci_ofw_bus_init(struct rt_pci_bus *bus);
rt_err_t rt_pci_ofw_device_init(struct rt_pci_device *pdev);
#else
rt_inline rt_err_t rt_pci_ofw_host_bridge_init(struct rt_ofw_node *dev_np, struct rt_pci_host_bridge *host_bridge)
{
    return RT_EOK;
}
rt_inline rt_err_t rt_pci_ofw_bus_init(struct rt_pci_bus *bus)
{
    return RT_EOK;
}
rt_inline rt_err_t rt_pci_ofw_device_init(struct rt_pci_device *pdev)
{
    return RT_EOK;
}
rt_inline int rt_pci_ofw_irq_parse_and_map(struct rt_pci_device *pdev, rt_uint8_t slot, rt_uint8_t pin)
{
    return -1;
}
rt_inline rt_err_t rt_pci_ofw_parse_ranges(struct rt_ofw_node *dev_np, struct rt_pci_host_bridge *host_bridge)
{
    return -RT_ENOSYS;
}
#endif /* RT_USING_OFW */

rt_inline void *rt_pci_iomap(struct rt_pci_device *pdev, int bar_idx)
{
    struct rt_pci_bus_resource *res = &pdev->resource[bar_idx];

    RT_ASSERT(bar_idx < RT_ARRAY_SIZE(pdev->resource));

    return rt_ioremap((void *)res->base, res->size);
}

rt_uint8_t rt_pci_irq_intx(struct rt_pci_device *pdev, rt_uint8_t pin);
rt_uint8_t rt_pci_irq_slot(struct rt_pci_device *pdev, rt_uint8_t *pinp);

void rt_pci_assign_irq(struct rt_pci_device *pdev);

void rt_pci_intx(struct rt_pci_device *pdev, rt_bool_t enable);
rt_bool_t rt_pci_check_and_mask_intx(struct rt_pci_device *pdev);
rt_bool_t rt_pci_check_and_unmask_intx(struct rt_pci_device *pdev);

rt_err_t rt_pci_region_setup(struct rt_pci_host_bridge *host_bridge);
struct rt_pci_bus_region *rt_pci_region_alloc(struct rt_pci_host_bridge *host_bridge,
        void **out_addr, rt_size_t size, rt_ubase_t flags, rt_bool_t mem64);

rt_err_t rt_pci_device_alloc_resource(struct rt_pci_host_bridge *host_bridge, struct rt_pci_device *pdev);

void rt_pci_enum_device(struct rt_pci_bus *bus, rt_bool_t (callback(struct rt_pci_device *)));

const struct rt_pci_device_id *rt_pci_match_id(struct rt_pci_device *pdev, const struct rt_pci_device_id *id);
const struct rt_pci_device_id *rt_pci_match_ids(struct rt_pci_device *pdev, const struct rt_pci_device_id *ids);

rt_err_t rt_pci_driver_register(struct rt_pci_driver *pdrv);
rt_err_t rt_pci_device_register(struct rt_pci_device *pdev);

#define RT_PCI_DRIVER_EXPORT(driver)    RT_DRIVER_EXPORT(driver, pci, BUILIN)

extern struct rt_spinlock rt_pci_lock;

#endif /* __PCI_H__ */
