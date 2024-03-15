#include <rtthread.h>

#ifdef PKG_USING_EXT4

#ifdef RT_USING_DFS
#include <dfs_posix.h>
#include <dfs_fs.h>

#define DBG_TAG "mnt"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

/*
 ******************************************************
 *
 * rk3568 NanoPi_5RC emmc whold size: 32GB.
 * ---------------------------------------------------
 * | partition name:    | kernel | rootfs | download  |
 * ---------------------------------------------------
 * | partition size:    | 100MB  | 8GB    | remainder |
 * ---------------------------------------------------
 * | emmc block device: | emmc0  | emmc1  | emmc2     |
 * ---------------------------------------------------
 *
 ******************************************************
 */

struct _mount_table
{
    char *dev_name;
    char *mount_point;
    char *fs_name;
    long rwflag;
    void *data;
};

const struct _mount_table _mount_table[] = {
    {"emmc1", "/", "ext", 0, 0},
    {"emmc0", "/kernel", "ext", 0, 0},
    {"emmc2", "/download", "ext", 0, 0},
};

static int _wait_device_ready(const char* devname)
{
    int k;

    for(k = 0; k < 10; k++)
    {
        if (rt_device_find(devname) != RT_NULL)
        {
            return 1;
        }
        rt_thread_mdelay(50);
    }

    return 0;
}

int mnt_init(void)
{
    int i;

    for (i = 0; i < sizeof(_mount_table) / sizeof(_mount_table[0]); i++)
    {
        if (_mount_table[i].dev_name && !_wait_device_ready(_mount_table[i].dev_name))
        {
            LOG_E("device %s find timeout", _mount_table[i].dev_name);
            continue;
        }

        if (dfs_mount(_mount_table[i].dev_name, _mount_table[i].mount_point,
                    _mount_table[i].fs_name, _mount_table[i].rwflag, _mount_table[i].data) != 0)
        {
            LOG_E("Dir %s %s mount failed!", _mount_table[i].mount_point,
                _mount_table[i].dev_name ? _mount_table[i].dev_name : _mount_table[i].fs_name);
        }
        else
        {
            LOG_I("Dir %s %s mount ok!", _mount_table[i].mount_point,
                _mount_table[i].dev_name ? _mount_table[i].dev_name : _mount_table[i].fs_name);
        }
    }

    rt_kprintf("file system initialization done!\n");
    return 0;
}
INIT_ENV_EXPORT(mnt_init);

#endif

#endif
