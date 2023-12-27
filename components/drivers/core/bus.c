/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2023-04-12     ErikChan     the first version
 */

#include <rtthread.h>
#include <drivers/core/bus.h>

#define DBG_TAG "bus"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

#if defined(RT_USING_DFS) && defined(RT_USING_DFS_DIRECTFS)
#include <dfs_fs.h>
#include <dfs_directfs.h>
#endif

static struct rt_bus bus_root =
{
    .name = "root",
    .children = RT_LIST_OBJECT_INIT(bus_root.children),
};

/**
 * @brief This function get the root bus
 */
rt_bus_t rt_bus_root(void)
{
    return &bus_root;
}

void rt_bus_lock(rt_bus_t bus)
{
    while (!rt_bus_trylock(bus))
    {
        rt_thread_yield();
    }
}

rt_bool_t rt_bus_trylock(rt_bus_t bus)
{
    rt_bool_t res = RT_FALSE;
    rt_ubase_t level = rt_spin_lock_irqsave(&bus->spinlock);

    if (rt_atomic_load(&bus->exclusive) == 0)
    {
        res = RT_TRUE;
        rt_atomic_store(&bus->exclusive, 1);
    }

    rt_spin_unlock_irqrestore(&bus->spinlock, level);

    return res;
}

void rt_bus_unlock(rt_bus_t bus)
{
    rt_atomic_store(&bus->exclusive, 0);
}

/**
 *  @brief This function loop the dev_list of the bus, and call fn in each loop
 *
 *  @param bus the target bus
 *
 *  @param drv the target drv to be matched
 *
 *  @param fn  the function callback in each loop
 *
 *  @return the error code, RT_EOK on added successfully.
 */
rt_err_t rt_bus_for_each_dev(rt_bus_t bus, rt_driver_t drv, int (*fn)(rt_driver_t drv, rt_device_t dev))
{
    rt_device_t dev;

    RT_ASSERT(bus != RT_NULL);
    RT_ASSERT(drv != RT_NULL);

    rt_bus_lock(bus);

    rt_list_for_each_entry(dev, &bus->dev_list, node)
    {
        fn(drv, dev);
    }

    rt_bus_unlock(bus);

    return RT_EOK;
}

/**
 *  @brief This function loop the drv_list of the bus, and call fn in each loop
 *
 *  @param bus the target bus
 *
 *  @param dev the target dev to be matched
 *
 *  @param fn  the function callback in each loop
 *
 *  @return the error code, RT_EOK on added successfully.
 */
rt_err_t rt_bus_for_each_drv(rt_bus_t bus, rt_device_t dev, int (*fn)(rt_driver_t drv, rt_device_t dev))
{
    rt_err_t err;
    rt_driver_t drv;

    RT_ASSERT(bus != RT_NULL);
    RT_ASSERT(dev != RT_NULL);

    if (rt_list_isempty(&bus->drv_list))
    {
        return RT_EOK;
    }

    err = -RT_ERROR;

    rt_bus_lock(bus);

    rt_list_for_each_entry(drv, &bus->drv_list, node)
    {
        if (fn(drv, dev))
        {
            err = -RT_EOK;

            break;
        }
    }

    rt_bus_unlock(bus);

    return err;
}

/**
 *  @brief This function add a bus to the root
 *
 *  @param bus_node the bus to be added
 *
 *  @return the error code, RT_EOK on added successfully.
 */
rt_err_t rt_bus_add(rt_bus_t bus_node)
{
    RT_ASSERT(bus_node != RT_NULL);

    bus_node->bus = &bus_root;
    rt_list_init(&bus_node->list);

    rt_bus_lock(bus_node);

    rt_list_insert_before(&bus_root.children, &bus_node->list);
#ifdef RT_USING_DFS_DIRECTFS
    do {
        static rt_object_t bus_obj = RT_NULL;

        if (!bus_obj)
        {
            bus_obj = dfs_directfs_find_object(RT_NULL, "bus");

            RT_ASSERT(bus_obj != RT_NULL);
        }

        dfs_directfs_create_link(bus_obj, &bus_node->parent, bus_node->name);
        dfs_directfs_create_link(&bus_node->parent, (rt_object_t)&bus_node->dev_list, "devices");
        dfs_directfs_create_link(&bus_node->parent, (rt_object_t)&bus_node->drv_list, "drivers");
    } while (0);
#endif

    rt_bus_unlock(bus_node);

    return RT_EOK;
}

/**
 *  @brief This function match the device and driver, probe them if match successed
 *
 *  @param drv the drv to match/probe
 *
 *  @param dev the dev to match/probe
 *
 *  @return the result of probe, 1 on added successfully.
 */
static int rt_bus_probe(rt_driver_t drv, rt_device_t dev)
{
    int ret = 0;
    rt_bus_t bus = drv->bus;

    if (!bus)
    {
        bus = dev->bus;
    }

    RT_ASSERT(bus != RT_NULL);

    if (!dev->drv && bus->match(drv, dev))
    {
        dev->drv = drv;

        ret = bus->probe(dev);

        if (ret)
        {
            dev->drv = RT_NULL;
        }
    }

    return ret;
}

/**
 *  @brief This function add a driver to the drv_list of a specific bus
 *
 *  @param bus the bus to add
 *
 *  @param drv the driver to be added
 *
 *  @return the error code, RT_EOK on added successfully.
 */
rt_err_t rt_bus_add_driver(rt_bus_t bus, rt_driver_t drv)
{
    RT_ASSERT(bus != RT_NULL);
    RT_ASSERT(drv != RT_NULL);

    drv->bus = bus;

    rt_bus_lock(bus);

    rt_list_insert_before(&bus->drv_list, &drv->node);

    rt_bus_unlock(bus);

    rt_bus_for_each_dev(drv->bus, drv, rt_bus_probe);

#ifdef RT_USING_DFS_DIRECTFS
    dfs_directfs_create_link((rt_object_t)&bus->drv_list, (rt_object_t)&drv->node, drv->name);
#endif

    return RT_EOK;
}

/**
 *  @brief This function add a device to the dev_list of a specific bus
 *
 *  @param bus the bus to add
 *
 *  @param dev the device to be added
 *
 *  @return the error code, RT_EOK on added successfully.
 */
rt_err_t rt_bus_add_device(rt_bus_t bus, rt_device_t dev)
{
    RT_ASSERT(bus != RT_NULL);
    RT_ASSERT(dev != RT_NULL);

    dev->bus = bus;

    rt_bus_lock(bus);

    rt_list_insert_before(&bus->dev_list, &dev->node);

    rt_bus_unlock(bus);
    
    rt_bus_for_each_drv(dev->bus, dev, rt_bus_probe);

#ifdef RT_USING_DFS_DIRECTFS
    dfs_directfs_create_link((rt_object_t)&bus->dev_list, (rt_object_t)&dev->parent, dev->parent.name);
#endif

    return RT_EOK;
}

/**
 *  @brief This function remove a driver from bus
 *
 *  @param drv the driver to be removed
 *
 *  @return the error code, RT_EOK on added successfully.
 */
rt_err_t rt_bus_remove_driver(rt_driver_t drv)
{
    RT_ASSERT(drv->bus != RT_NULL);

    LOG_D("Bus(%s) remove driver %s", drv->bus->name, drv->name);

    rt_list_remove(&drv->node);

    return RT_EOK;
}

/**
 *  @brief This function remove a device from bus
 *
 *  @param dev the device to be removed
 *
 *  @return the error code, RT_EOK on added successfully.
 */
rt_err_t rt_bus_remove_device(rt_device_t dev)
{
    RT_ASSERT(dev->bus != RT_NULL);

    LOG_D("Bus(%s) remove device %s", dev->bus->name, dev->name);

    rt_list_remove(&dev->node);

    return RT_EOK;
}

/**
 *  @brief This function find a bus by name
 *  @param bus the name to be finded
 *
 *  @return the bus finded by name.
 */
rt_bus_t rt_bus_find_by_name(char *name)
{
    rt_bus_t bus = RT_NULL;
    struct rt_list_node *node = RT_NULL;

    if (!rt_list_isempty(&bus_root.children))
    {
        rt_list_for_each(node, &bus_root.children)
        {
            bus = rt_list_entry(node, struct rt_bus, list);

            if (!rt_strncmp(bus->name, name, RT_NAME_MAX))
            {
                return bus;
            }
        }
    }

    return bus;
}

/**
 *  @brief This function transfer dev_list and drv_list to the other bus
 *
 *  @param new_bus the bus to transfer
 *
 *  @param dev the target device
 *
 *  @return the error code, RT_EOK on added successfully.
 */
rt_err_t rt_bus_reload_driver_device(rt_bus_t new_bus, rt_device_t dev)
{
    RT_ASSERT(new_bus != RT_NULL);
    RT_ASSERT(dev != RT_NULL);

    rt_bus_lock(new_bus);

    rt_list_remove(&dev->node);
    rt_list_insert_before(&new_bus->dev_list, &dev->node);

    rt_list_remove(&dev->drv->node);
    rt_list_insert_before(&new_bus->drv_list, &dev->drv->node);

    rt_bus_unlock(new_bus);

    return RT_EOK;
}

/**
 *  @brief This function register a bus
 *  @param bus the bus to be registered
 *
 *  @return the error code, RT_EOK on registeration successfully.
 */
rt_err_t rt_bus_register(rt_bus_t bus)
{
    rt_list_init(&bus->children);
    rt_list_init(&bus->dev_list);
    rt_list_init(&bus->drv_list);

    rt_spin_lock_init(&bus->spinlock);
    rt_atomic_store(&bus->exclusive, 0);

    rt_bus_add(bus);

    return RT_EOK;
}
