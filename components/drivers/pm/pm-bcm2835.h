/*
 * Copyright (c) 2006-2023, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2023-02-25     GuEe-GUI     the first version
 */

#ifndef __PM_BCM2835_H__
#define __PM_BCM2835_H__

struct bcm2835_pm
{
    void *base;
    void *asb;
    void *rpivid_asb;

    void *ofw_node;
};

#endif /* __PM_BCM2835_H__ */
