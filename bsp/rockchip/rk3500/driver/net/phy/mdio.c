
/*
 * Copyright (c) 2013-2022, Shanghai Real-Thread Electronic Technology Co.,Ltd
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-6-2     songchao     the first version
 */

#include "phy.h"
#include <rtthread.h>

#define DBG_TAG "drv.mdio"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

static struct rt_list_node mdio_devs;

struct mdio_bus *mdio_dev_get_by_name(const char *bus_name)
{
    struct rt_list_node *entry;
    struct mdio_bus *dev;
    if (!bus_name) 
    {
        LOG_E("RT_NULL device name!\n");
        return RT_NULL;
    }
    rt_list_for_each(entry, &mdio_devs) 
    {
        dev = rt_list_entry(entry, struct mdio_bus, link);
        if (rt_strcmp(dev->name, bus_name) == 0)
        {
            return dev;
        }
    }
    return RT_NULL;
}

void list_mdio_dev(void)
{
    struct rt_list_node *entry;
    struct mdio_bus *dev;

    rt_list_for_each(entry, &mdio_devs) 
    {
        dev = rt_list_entry(entry, struct mdio_bus, link);
        rt_kprintf("'%s' ", dev->name);
    }
    rt_kprintf("\n");
}
MSH_CMD_EXPORT(list_mdio_dev,list all phy device)



struct mdio_bus *mdio_alloc(void)
{
    struct mdio_bus *bus;

    bus = rt_malloc(sizeof(struct mdio_bus));
    if (!bus)
    {
        return RT_NULL;
    }

    rt_memset(bus, 0, sizeof(struct mdio_bus));

    rt_list_init(&bus->link);
    rt_list_init(&bus->phy_driver_list);

    bus->mutex = rt_mutex_create("mdio_mutex", RT_IPC_FLAG_FIFO);
    return bus;
}

void mdio_free(struct mdio_bus *bus)
{
    rt_free(bus);
}

int mdio_register(struct mdio_bus *bus)
{
    if (!bus)
    {
        return -1;
    }
    if(!bus->read || !bus->write)
    {
        LOG_E("bus->read or bus->write is NULL bus cannot be register");
        return -1;
    }

    if (mdio_dev_get_by_name(bus->name)) 
    {
        LOG_E("%s is already exist\n",bus->name);
        return -1;
    }

    rt_list_insert_after(&mdio_devs,&bus->link);
    rt_device_register((rt_device_t)bus, bus->name, RT_DEVICE_FLAG_RDWR);
    LOG_D("mdio bus register success\n");
    return 0;
}

int mdio_unregister(struct mdio_bus *bus)
{
    if (!bus)
    {
        return -1;
    }
    rt_list_remove(&bus->link);
    rt_device_unregister((rt_device_t)bus);
    return 0;
}

void mdio_list_devices(void)
{
    struct rt_list_node *entry;

    rt_list_for_each(entry, &mdio_devs) 
    {
        int i;
        struct mdio_bus *bus = rt_list_entry(entry, struct mdio_bus, link);

        LOG_D("%s:\n", bus->name);

        for (i = 0; i < PHY_MAX_ADDR; i++) 
        {
            struct phy_device *phydev = bus->phydev_list[i];
            if (phydev) 
            {
                rt_kprintf("%x - %s", i, phydev->drv->name);
                rt_kprintf(" <--> %s\n", phydev->parent.parent.name);
            }
        }
    }
}

struct phy_device *mdio_get_phydev_by_name(const char *phydev_name)
{
    struct rt_list_node *entry;
    struct mdio_bus *bus;

    rt_list_for_each(entry, &mdio_devs) 
    {
        int i;
        bus = rt_list_entry(entry, struct mdio_bus, link);
        for (i = 0; i < PHY_MAX_ADDR; i++) 
        {
            if (!bus->phydev_list[i])
            {
                continue;
            }
            if (rt_strcmp(bus->phydev_list[i]->parent.parent.name, phydev_name) == 0)
            {
                return bus->phydev_list[i];
            }
        }
    }

    LOG_D("%s is not exist\n", phydev_name);
    return RT_NULL;
}

int mdio_phy_init(void)
{
    rt_list_init(&mdio_devs);
    return 0;
}
INIT_PREV_EXPORT(mdio_phy_init);
