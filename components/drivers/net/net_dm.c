/*
 * Copyright (c) 2006-2022, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2023-12-04     zhaomaosheng first version
 */

#include "net.h"

static const char * const phy_interface_strings[] = {
	[PHY_INTERFACE_MODE_MII]		= "mii",
	[PHY_INTERFACE_MODE_GMII]		= "gmii",
	[PHY_INTERFACE_MODE_SGMII]		= "sgmii",
	[PHY_INTERFACE_MODE_SGMII_2500]		= "sgmii-2500",
	[PHY_INTERFACE_MODE_QSGMII]		= "qsgmii",
	[PHY_INTERFACE_MODE_TBI]		= "tbi",
	[PHY_INTERFACE_MODE_RMII]		= "rmii",
	[PHY_INTERFACE_MODE_RGMII]		= "rgmii",
	[PHY_INTERFACE_MODE_RGMII_ID]		= "rgmii-id",
	[PHY_INTERFACE_MODE_RGMII_RXID]		= "rgmii-rxid",
	[PHY_INTERFACE_MODE_RGMII_TXID]		= "rgmii-txid",
	[PHY_INTERFACE_MODE_NONE]		= "",
};

int phy_get_interface_by_name(const char *str)
{
	int i;

	for (i = 0; i < PHY_INTERFACE_MODE_NONE; i++) {
		if (!strcmp(str, phy_interface_strings[i]))
			return i;
	}

	return -1;
}

phy_interface_t rt_ofw_net_get_interface(const struct rt_device *dev)
{
	const char *phy_mode;
	phy_interface_t interface = PHY_INTERFACE_MODE_NONE;

	rt_ofw_prop_read_string(dev->ofw_node, "phy-mode", &phy_mode);
	if (phy_mode)
		interface = phy_get_interface_by_name(phy_mode);

	return interface;
}
