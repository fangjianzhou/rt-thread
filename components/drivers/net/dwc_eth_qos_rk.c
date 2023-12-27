// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright Contributors to the U-Boot project.
 *
 * rk_gmac_ops ported from linux drivers/net/ethernet/stmicro/stmmac/dwmac-rk.c
 *
 * Ported code is intentionally left as close as possible with linux counter
 * part in order to simplify future porting of fixes and support for other SoCs.
 */

#include <rtthread.h>

#include "dwc_eth_qos.h"
#include "net.h"

#define DBG_TAG "drv.net"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

struct rk_gmac_ops {
	const char *compatible;
	int (*set_to_rgmii)(struct rt_device *dev,
			    int tx_delay, int rx_delay);
	int (*set_to_rmii)(struct rt_device *dev);
	int (*set_gmac_speed)(struct rt_device *dev);
	void (*set_clock_selection)(struct rt_device *dev, bool enable);
};

struct rockchip_platform_data {
	const struct rk_gmac_ops *ops;
	int id;
	bool clock_input;
	void *grf;
	void *php_grf;
};

#define HIWORD_UPDATE(val, mask, shift) \
		((val) << (shift) | (mask) << ((shift) + 16))

#define GRF_BIT(nr)	(RT_BIT(nr) | RT_BIT((nr) + 16))
#define GRF_CLR_BIT(nr)	(RT_BIT((nr) + 16))


#define RK3568_GRF_GMAC0_CON0		0x0380
#define RK3568_GRF_GMAC0_CON1		0x0384
#define RK3568_GRF_GMAC1_CON0		0x0388
#define RK3568_GRF_GMAC1_CON1		0x038c

/* RK3568_GRF_GMAC0_CON1 && RK3568_GRF_GMAC1_CON1 */
#define RK3568_GMAC_PHY_INTF_SEL_RGMII	\
		(GRF_BIT(4) | GRF_CLR_BIT(5) | GRF_CLR_BIT(6))
#define RK3568_GMAC_PHY_INTF_SEL_RMII	\
		(GRF_CLR_BIT(4) | GRF_CLR_BIT(5) | GRF_BIT(6))
#define RK3568_GMAC_FLOW_CTRL			GRF_BIT(3)
#define RK3568_GMAC_FLOW_CTRL_CLR		GRF_CLR_BIT(3)
#define RK3568_GMAC_RXCLK_DLY_ENABLE		GRF_BIT(1)
#define RK3568_GMAC_RXCLK_DLY_DISABLE		GRF_CLR_BIT(1)
#define RK3568_GMAC_TXCLK_DLY_ENABLE		GRF_BIT(0)
#define RK3568_GMAC_TXCLK_DLY_DISABLE		GRF_CLR_BIT(0)

/* RK3568_GRF_GMAC0_CON0 && RK3568_GRF_GMAC1_CON0 */
#define RK3568_GMAC_CLK_RX_DL_CFG(val)	HIWORD_UPDATE(val, 0x7F, 8)
#define RK3568_GMAC_CLK_TX_DL_CFG(val)	HIWORD_UPDATE(val, 0x7F, 0)

static int rk3568_set_to_rgmii(struct rt_device *dev,
			       int tx_delay, int rx_delay)
{
	struct eqos_eth_device *eqos = (struct eqos_eth_device *)dev;
	struct eqos_eth_data *pdata = &eqos->pdata;
	struct rockchip_platform_data *data = pdata->priv_pdata;
	rt_uint32_t con0, con1;

	con0 = (data->id == 1) ? RK3568_GRF_GMAC1_CON0 :
				 RK3568_GRF_GMAC0_CON0;
	con1 = (data->id == 1) ? RK3568_GRF_GMAC1_CON1 :
				 RK3568_GRF_GMAC0_CON1;
	writel(RK3568_GMAC_CLK_RX_DL_CFG(rx_delay) |
		     RK3568_GMAC_CLK_TX_DL_CFG(tx_delay), data->grf + con0);
	writel(RK3568_GMAC_PHY_INTF_SEL_RGMII |
		     RK3568_GMAC_RXCLK_DLY_ENABLE |
		     RK3568_GMAC_TXCLK_DLY_ENABLE, data->grf + con1);
	return 0;
}

static int rk3568_set_to_rmii(struct rt_device *dev)
{
	struct eqos_eth_device *eqos = (struct eqos_eth_device *)dev;
	struct eqos_eth_data *pdata = &eqos->pdata;
	struct rockchip_platform_data *data = pdata->priv_pdata;
	rt_uint32_t con1;

	con1 = (data->id == 1) ? RK3568_GRF_GMAC1_CON1 :
				 RK3568_GRF_GMAC0_CON1;
	writel(RK3568_GMAC_PHY_INTF_SEL_RMII, data->grf + con1);
	//regmap_write(data->grf, con1, RK3568_GMAC_PHY_INTF_SEL_RMII);

	return 0;
}

static int rk3568_set_gmac_speed(struct rt_device *dev)
{
	struct eqos_eth_device *eqos = (struct eqos_eth_device *)dev;
	rt_ubase_t rate;
	int ret;

	switch (eqos->phy->speed) {
	case SPEED_10:
		rate = 2500000;
		break;
	case SPEED_100:
		rate = 25000000;
		break;
	case SPEED_1000:
		rate = 125000000;
		break;
	default:
		return -EINVAL;
	}

	ret = rt_clk_set_rate(eqos->clk_tx, rate);
	if (ret < 0)
		return ret;

	return 0;
}
/* sys_grf */
#define RK3588_GRF_GMAC_CON7			0x031c
#define RK3588_GRF_GMAC_CON8			0x0320
#define RK3588_GRF_GMAC_CON9			0x0324

#define RK3588_GMAC_RXCLK_DLY_ENABLE(id)	GRF_BIT(2 * (id) + 3)
#define RK3588_GMAC_RXCLK_DLY_DISABLE(id)	GRF_CLR_BIT(2 * (id) + 3)
#define RK3588_GMAC_TXCLK_DLY_ENABLE(id)	GRF_BIT(2 * (id) + 2)
#define RK3588_GMAC_TXCLK_DLY_DISABLE(id)	GRF_CLR_BIT(2 * (id) + 2)

#define RK3588_GMAC_CLK_RX_DL_CFG(val)		HIWORD_UPDATE(val, 0xFF, 8)
#define RK3588_GMAC_CLK_TX_DL_CFG(val)		HIWORD_UPDATE(val, 0xFF, 0)

/* php_grf */
#define RK3588_GRF_GMAC_CON0			0x0008
#define RK3588_GRF_CLK_CON1			0x0070

#define RK3588_GMAC_PHY_INTF_SEL_RGMII(id)	\
	(GRF_BIT(3 + (id) * 6) | GRF_CLR_BIT(4 + (id) * 6) | GRF_CLR_BIT(5 + (id) * 6))
#define RK3588_GMAC_PHY_INTF_SEL_RMII(id)	\
	(GRF_CLR_BIT(3 + (id) * 6) | GRF_CLR_BIT(4 + (id) * 6) | GRF_BIT(5 + (id) * 6))

#define RK3588_GMAC_CLK_RMII_MODE(id)		GRF_BIT(5 * (id))
#define RK3588_GMAC_CLK_RGMII_MODE(id)		GRF_CLR_BIT(5 * (id))

#define RK3588_GMAC_CLK_SELET_CRU(id)		GRF_BIT(5 * (id) + 4)
#define RK3588_GMAC_CLK_SELET_IO(id)		GRF_CLR_BIT(5 * (id) + 4)

#define RK3588_GMAC_CLK_RMII_DIV2(id)		GRF_BIT(5 * (id) + 2)
#define RK3588_GMAC_CLK_RMII_DIV20(id)		GRF_CLR_BIT(5 * (id) + 2)

#define RK3588_GMAC_CLK_RGMII_DIV1(id)		\
			(GRF_CLR_BIT(5 * (id) + 2) | GRF_CLR_BIT(5 * (id) + 3))
#define RK3588_GMAC_CLK_RGMII_DIV5(id)		\
			(GRF_BIT(5 * (id) + 2) | GRF_BIT(5 * (id) + 3))
#define RK3588_GMAC_CLK_RGMII_DIV50(id)		\
			(GRF_CLR_BIT(5 * (id) + 2) | GRF_BIT(5 * (id) + 3))

#define RK3588_GMAC_CLK_RMII_GATE(id)		GRF_BIT(5 * (id) + 1)
#define RK3588_GMAC_CLK_RMII_NOGATE(id)		GRF_CLR_BIT(5 * (id) + 1)

static int rk3588_set_to_rgmii(struct rt_device *dev,
			       int tx_delay, int rx_delay)
{
	struct eqos_eth_device *eqos = (struct eqos_eth_device *)dev;
	struct eqos_eth_data *pdata = &eqos->pdata;
	struct rockchip_platform_data *data = pdata->priv_pdata;
	rt_uint32_t offset_con, id = data->id;

	offset_con = data->id == 1 ? RK3588_GRF_GMAC_CON9 :
				     RK3588_GRF_GMAC_CON8;

	writel(RK3588_GMAC_PHY_INTF_SEL_RGMII(id), data->php_grf + RK3588_GRF_GMAC_CON0);
	writel(RK3588_GMAC_CLK_RGMII_MODE(id), data->php_grf + RK3588_GRF_CLK_CON1);
	writel(RK3588_GMAC_RXCLK_DLY_ENABLE(id) | RK3588_GMAC_TXCLK_DLY_ENABLE(id), data->grf + RK3588_GRF_GMAC_CON7);
	writel(RK3588_GMAC_CLK_RX_DL_CFG(rx_delay) | RK3588_GMAC_CLK_TX_DL_CFG(tx_delay), data->grf + offset_con);

	return 0;
}

static int rk3588_set_to_rmii(struct rt_device *dev)
{
	struct eqos_eth_device *eqos = (struct eqos_eth_device *)dev;
	struct eqos_eth_data *pdata = &eqos->pdata;
	struct rockchip_platform_data *data = pdata->priv_pdata;

	writel(RK3588_GMAC_PHY_INTF_SEL_RMII(data->id), data->php_grf + RK3588_GRF_GMAC_CON0);
	writel(RK3588_GMAC_CLK_RMII_MODE(data->id), data->php_grf + RK3588_GRF_CLK_CON1);

	return 0;
}

static int rk3588_set_gmac_speed(struct rt_device *dev)
{
	struct eqos_eth_device *eqos = (struct eqos_eth_device *)dev;
	struct eqos_eth_data *pdata = &eqos->pdata;
	struct rockchip_platform_data *data = pdata->priv_pdata;
	rt_uint32_t val = 0, id = data->id;

	switch (eqos->phy->speed) {
	case SPEED_10:
		if (pdata->phy_interface == PHY_INTERFACE_MODE_RMII)
			val = RK3588_GMAC_CLK_RMII_DIV20(id);
		else
			val = RK3588_GMAC_CLK_RGMII_DIV50(id);
		break;
	case SPEED_100:
		if (pdata->phy_interface == PHY_INTERFACE_MODE_RMII)
			val = RK3588_GMAC_CLK_RMII_DIV2(id);
		else
			val = RK3588_GMAC_CLK_RGMII_DIV5(id);
		break;
	case SPEED_1000:
		if (pdata->phy_interface != PHY_INTERFACE_MODE_RMII)
			val = RK3588_GMAC_CLK_RGMII_DIV1(id);
		else
			return -RT_EINVAL;
		break;
	default:
		return -RT_EINVAL;
	}
	writel(val, data->php_grf + RK3588_GRF_CLK_CON1);
	return 0;
}

static void rk3588_set_clock_selection(struct rt_device *dev, bool enable)
{
	struct eqos_eth_device *eqos = (struct eqos_eth_device *)dev;
	struct eqos_eth_data *pdata = &eqos->pdata;
	struct rockchip_platform_data *data = pdata->priv_pdata;

	rt_uint32_t val = data->clock_input ? RK3588_GMAC_CLK_SELET_IO(data->id) :
				      RK3588_GMAC_CLK_SELET_CRU(data->id);

	val |= enable ? RK3588_GMAC_CLK_RMII_NOGATE(data->id) :
			RK3588_GMAC_CLK_RMII_GATE(data->id);
	
	writel(val, data->php_grf + RK3588_GRF_CLK_CON1);
}

static const struct rk_gmac_ops rk_gmac_ops[] = {
	{
		.compatible = "rockchip,rk3568-gmac",
		.set_to_rgmii = rk3568_set_to_rgmii,
		.set_to_rmii = rk3568_set_to_rmii,
		.set_gmac_speed = rk3568_set_gmac_speed,
	},
	{
		.compatible = "rockchip,rk3588-gmac",
		.set_to_rgmii = rk3588_set_to_rgmii,
		.set_to_rmii = rk3588_set_to_rmii,
		.set_gmac_speed = rk3588_set_gmac_speed,
		.set_clock_selection = rk3588_set_clock_selection,
	},
	{ }
};

static const struct rk_gmac_ops *get_rk_gmac_ops(struct rt_device *dev)
{
	const struct rk_gmac_ops *ops = rk_gmac_ops;

	while (ops->compatible) {
		if (rt_ofw_node_is_compatible(dev->ofw_node, ops->compatible))
			return ops;
		ops++;
	}

	return NULL;
}

static int eqos_probe_resources_rk(struct rt_device *dev)
{
	struct eqos_eth_device *eqos = (struct eqos_eth_device *)dev;
	struct eqos_eth_data *pdata = &eqos->pdata;
	struct rockchip_platform_data *data;
	const char *clock_in_out;
	int ret = RT_EOK;

	data = rt_malloc(sizeof(struct rockchip_platform_data));
	if (!data)
		return -RT_ENOMEM;

	data->ops = get_rk_gmac_ops(dev);
	if (!data->ops) {
		ret = -RT_EINVAL;
		goto err_free;
	}

	data->php_grf = rt_syscon_find_by_ofw_compatible("rockchip,rk3588-php-grf")->iomem_base;
	data->grf = rt_syscon_find_by_ofw_compatible("rockchip,rk3588-sys-grf")->iomem_base;

	data->id = rt_ofw_get_alias_id(dev->ofw_node, "ethernet");
	eqos->id = data->id;
	pdata->priv_pdata = data;
	pdata->phy_interface = eqos->config.interface(dev);
	pdata->max_speed = eqos->max_speed;

	if (pdata->phy_interface == PHY_INTERFACE_MODE_NONE)
	{
		LOG_E("Invalid PHY interface\n");
		ret = -RT_EINVAL;
		goto err_free;
	}

	eqos->reset_ctl = rt_reset_control_get_by_name(dev ,"stmmaceth");

	rt_reset_control_assert(eqos->reset_ctl);

	eqos->clk_master_bus = rt_clk_get_by_name(dev, "stmmaceth");
	if (!eqos->clk_master_bus)
	{
		LOG_E("clk_get_by_name(stmmaceth) failed");
		goto err_release_resets;
	}
	if (rt_ofw_node_is_compatible(dev->ofw_node, "rockchip,rk3568-gmac") ){
		eqos->clk_tx = rt_clk_get_by_name(dev, "clk_mac_speed");
		if (!eqos->clk_tx) {
			LOG_E("clk_get_by_name(clk_mac_speed) failed");
			goto err_free_clk_master_bus;
		}
	}
	rt_ofw_prop_read_string(dev->ofw_node, "clock_in_out", &clock_in_out);
	if (clock_in_out && !rt_strcmp(clock_in_out, "input"))
		data->clock_input = RT_TRUE;
	else
		data->clock_input = RT_FALSE;

	/* snps,reset props are deprecated, do bare minimum to support them */
	rt_uint8_t mode, value;
	eqos->phy_reset_gpio = rt_ofw_get_named_pin(dev->ofw_node, "snps,reset", 0, &mode, &value);
	rt_pin_mode(eqos->phy_reset_gpio, mode);
	eqos->phy_reset_active_low = rt_ofw_prop_read_bool(dev->ofw_node, "snps,reset-active-low");
	if (eqos->phy_reset_gpio >= 0)
		ret = rt_ofw_prop_read_u32_array_index(dev->ofw_node, "snps,reset-delays-us", 0, 3, eqos->reset_delays);
	else
		LOG_W("gpio_request_by_name(snps,reset-gpio) failed: %d",ret);
	return 0;

err_free_clk_master_bus:
	rt_clk_put(eqos->clk_master_bus);
err_release_resets:
	rt_reset_control_put(eqos->reset_ctl);
err_free:
	rt_free(data);

	return ret;
}

static int eqos_remove_resources_rk(struct rt_device *dev)
{
	struct eqos_eth_device *eqos = (struct eqos_eth_device *)dev;
	struct eqos_eth_data *pdata = &eqos->pdata;
	struct rockchip_platform_data *data = pdata->priv_pdata;

	rt_clk_put(eqos->clk_tx);
	rt_clk_put(eqos->clk_master_bus);
	rt_reset_control_put(eqos->reset_ctl);
	rt_free(data);

	return 0;
}

static int eqos_stop_resets_rk(struct rt_device *dev)
{
	struct eqos_eth_device *eqos = (struct eqos_eth_device *)dev;

	return rt_reset_control_assert(eqos->reset_ctl);
}

static int eqos_start_resets_rk(struct rt_device *dev)
{
	struct eqos_eth_device *eqos = (struct eqos_eth_device *)dev;

	return rt_reset_control_deassert(eqos->reset_ctl);
}

static int eqos_stop_clks_rk(struct rt_device *dev)
{
	struct eqos_eth_device *eqos = (struct eqos_eth_device *)dev;
	struct eqos_eth_data *pdata = &eqos->pdata;
	struct rockchip_platform_data *data = pdata->priv_pdata;

	if (data->ops->set_clock_selection)
		data->ops->set_clock_selection(dev, RT_FALSE);

	return 0;
}

static int eqos_start_clks_rk(struct rt_device *dev)
{
	struct eqos_eth_device *eqos = (struct eqos_eth_device *)dev;
	struct eqos_eth_data *pdata = &eqos->pdata;
	struct rockchip_platform_data *data = pdata->priv_pdata;
	rt_uint32_t tx_delay, rx_delay;
	int ret;

	rt_hw_us_delay(eqos->reset_delays[1]);

	rt_pin_write(eqos->phy_reset_gpio, eqos->phy_reset_active_low ? 1 : 0);

	rt_hw_us_delay(eqos->reset_delays[2]);

	if (data->ops->set_clock_selection)
		data->ops->set_clock_selection(dev, RT_TRUE);

	ret = rt_ofw_prop_read_u32(dev->ofw_node, "tx_delay", &tx_delay);
	if (ret)
		tx_delay = 0x30;
	ret = rt_ofw_prop_read_u32(dev->ofw_node, "rx_delay", &rx_delay);
	if (ret)
		rx_delay = 0x10;

	switch (pdata->phy_interface) {
	case PHY_INTERFACE_MODE_RGMII:
		return data->ops->set_to_rgmii(dev, tx_delay, rx_delay);
	case PHY_INTERFACE_MODE_RGMII_ID:
		return data->ops->set_to_rgmii(dev, 0, 0);
	case PHY_INTERFACE_MODE_RGMII_RXID:
		return data->ops->set_to_rgmii(dev, tx_delay, 0);
	case PHY_INTERFACE_MODE_RGMII_TXID:
		return data->ops->set_to_rgmii(dev, 0, rx_delay);
	case PHY_INTERFACE_MODE_RMII:
		return data->ops->set_to_rmii(dev);
	}

	return -RT_EINVAL;
}

static int eqos_set_tx_clk_speed_rk(struct rt_device *dev)
{
	struct eqos_eth_device *eqos = (struct eqos_eth_device *)dev;
	struct eqos_eth_data *pdata = &eqos->pdata;
	struct rockchip_platform_data *data = pdata->priv_pdata;

	return data->ops->set_gmac_speed(dev);
}

static rt_ubase_t eqos_get_tick_clk_rate_rk(struct rt_device *dev)
{
	struct eqos_eth_device *eqos = (struct eqos_eth_device *)dev;

	return rt_clk_get_rate(eqos->clk_master_bus);
}

static struct eqos_ops eqos_rockchip_ops = {
	.eqos_inval_buffer = eqos_inval_buffer_generic,
	.eqos_probe_resources = eqos_probe_resources_rk,
	.eqos_remove_resources = eqos_remove_resources_rk,
	.eqos_stop_resets = eqos_stop_resets_rk,
	.eqos_start_resets = eqos_start_resets_rk,
	.eqos_stop_clks = eqos_stop_clks_rk,
	.eqos_start_clks = eqos_start_clks_rk,
	.eqos_calibrate_pads = eqos_null_ops,
	.eqos_disable_calibration = eqos_null_ops,
	.eqos_set_tx_clk_speed = eqos_set_tx_clk_speed_rk,
	.eqos_get_enetaddr = eqos_null_ops,
	.eqos_get_tick_clk_rate = eqos_get_tick_clk_rate_rk,
};

struct eqos_config eqos_rockchip_config = {
	.reg_access_always_ok = RT_FALSE,
	.mdio_wait = 10,
	.swr_wait = 50,
	.config_mac = EQOS_MAC_RXQ_CTRL0_RXQ0EN_ENABLED_DCB,
	.config_mac_mdio = EQOS_MAC_MDIO_ADDRESS_CR_100_150,
	.axi_bus_width = EQOS_AXI_WIDTH_64,
	.interface = rt_ofw_net_get_interface,
	.ops = &eqos_rockchip_ops,
};
