/*
 * Copyright (c) 2006-2022, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-12-06     GuEe-GUI     first version
 */

#include <rtatomic.h>
#include "rtc_dm.h"

#define DBG_TAG "rtc.dm"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

int rtc_dev_set_name(struct rt_device *rtc_dev)
{
    int id;
    static volatile rt_atomic_t uid = 1;

    RT_ASSERT(rtc_dev != RT_NULL)

    if (rt_device_find("rtc"))
    {
        id = (int)rt_hw_atomic_add(&uid, 1);

        return rt_dm_dev_set_name(rtc_dev, "rtc%u", id);
    }
    else
    {
        return rt_dm_dev_set_name(rtc_dev, "rtc");
    }
}
