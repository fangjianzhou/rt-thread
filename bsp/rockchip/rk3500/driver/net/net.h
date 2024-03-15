/*
 * Copyright (c) 2006-2022, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2023-12-04     zhaomaosheng first version
 */

#ifndef __NET_H
#define __NET_H

#include <rthw.h>
#include <rtthread.h>
#include <rtdevice.h>

typedef enum 
{
    PHY_INTERFACE_MODE_MII,
    PHY_INTERFACE_MODE_GMII,
    PHY_INTERFACE_MODE_SGMII,
    PHY_INTERFACE_MODE_SGMII_2500,
    PHY_INTERFACE_MODE_QSGMII,
    PHY_INTERFACE_MODE_TBI,
    PHY_INTERFACE_MODE_RMII,
    PHY_INTERFACE_MODE_RGMII,
    PHY_INTERFACE_MODE_RGMII_ID,
    PHY_INTERFACE_MODE_RGMII_RXID,
    PHY_INTERFACE_MODE_RGMII_TXID,
    PHY_INTERFACE_MODE_NONE,
} phy_interface_t;

#ifdef RT_USING_OFW
phy_interface_t rt_ofw_net_get_interface(const struct rt_device *dev);
#endif

#endif
