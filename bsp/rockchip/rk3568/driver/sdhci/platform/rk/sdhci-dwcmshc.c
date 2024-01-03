/*
 * Copyright (c) 2006-2022, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 */

#include <dt-bindings/size.h>
#include "../../sdhci-pltfm.h"
#define SDHCI_DWCMSHC_ARG2_STUFF        RT_GENMASK(31, 16)

/* DWCMSHC specific Mode Select value */
#define DWCMSHC_CTRL_HS400              0x7

/* DWC IP vendor area 1 pointer */
#define DWCMSHC_P_VENDOR_AREA1          0xe8
#define DWCMSHC_AREA1_MASK              RT_GENMASK(11, 0)
/* Offset inside the  vendor area 1 */
#define DWCMSHC_HOST_CTRL3              0x8
#define DWCMSHC_EMMC_CONTROL            0x2c
#define DWCMSHC_CARD_IS_EMMC            RT_BIT(0)
#define DWCMSHC_ENHANCED_STROBE         RT_BIT(8)
#define DWCMSHC_EMMC_ATCTRL             0x40

/* Rockchip specific Registers */
#define DWCMSHC_EMMC_DLL_CTRL           0x800
#define DWCMSHC_EMMC_DLL_RXCLK          0x804
#define DWCMSHC_EMMC_DLL_TXCLK          0x808
#define DWCMSHC_EMMC_DLL_STRBIN         0x80c
#define DECMSHC_EMMC_DLL_CMDOUT         0x810
#define DWCMSHC_EMMC_DLL_STATUS0        0x840
#define DWCMSHC_EMMC_DLL_START          RT_BIT(0)
#define DWCMSHC_EMMC_DLL_LOCKED         RT_BIT(8)
#define DWCMSHC_EMMC_DLL_TIMEOUT        RT_BIT(9)
#define DWCMSHC_EMMC_DLL_RXCLK_SRCSEL   29
#define DWCMSHC_EMMC_DLL_START_POINT    16
#define DWCMSHC_EMMC_DLL_INC            8
#define DWCMSHC_EMMC_DLL_BYPASS         RT_BIT(24)
#define DWCMSHC_EMMC_DLL_DLYENA         RT_BIT(27)
#define DLL_TXCLK_TAPNUM_DEFAULT        0x10
#define DLL_TXCLK_TAPNUM_90_DEGREES     0xA
#define DLL_TXCLK_TAPNUM_FROM_SW        RT_BIT(24)
#define DLL_STRBIN_TAPNUM_DEFAULT       0x8
#define DLL_STRBIN_TAPNUM_FROM_SW       RT_BIT(24)
#define DLL_STRBIN_DELAY_NUM_SEL        RT_BIT(26)
#define DLL_STRBIN_DELAY_NUM_OFFSET     16
#define DLL_STRBIN_DELAY_NUM_DEFAULT    0x16
#define DLL_RXCLK_NO_INVERTER           1
#define DLL_RXCLK_INVERTER              0
#define DLL_CMDOUT_TAPNUM_90_DEGREES    0x8
#define DLL_RXCLK_ORI_GATE              RT_BIT(31)
#define DLL_CMDOUT_TAPNUM_FROM_SW       RT_BIT(24)
#define DLL_CMDOUT_SRC_CLK_NEG          RT_BIT(28)
#define DLL_CMDOUT_EN_SRC_CLK_NEG       RT_BIT(29)

#define USEC_PER_MSEC               1000L

#define DLL_LOCK_WO_TMOUT(x) \
        ((((x) & DWCMSHC_EMMC_DLL_LOCKED) == DWCMSHC_EMMC_DLL_LOCKED) && \
            (((x) & DWCMSHC_EMMC_DLL_TIMEOUT) == 0))

#define BOUNDARY_OK(addr, len) \
        ((addr | ((128 * SIZE_MB) - 1)) == ((addr + len - 1) | ((128 * SIZE_MB) - 1)))

struct sdwcmshc_soc_data
{
    const struct rt_mmcsd_host_ops *ops;
};

struct dwcmshc_priv
{
    struct rt_clk *bus_clk;
    /* P_VENDOR_SPECIFIC_AREA reg */
    int vendor_specific_area1;
    void *priv;
};

enum dwcmshc_rk_type
{
    DWCMSHC_RK3568,
    DWCMSHC_RK3588,
};

struct rk35xx_priv
{
    /* Rockchip specified optional clocks */
    struct rt_clk_array *clk_arr;
    struct rt_reset_control *reset;
    enum dwcmshc_rk_type devtype;
    rt_uint8_t txclk_tapnum;
};
static void dwcmshc_rk3568_set_clock(struct sdhci_host *host, unsigned int clock)
{
    struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
    struct dwcmshc_priv *dwc_priv = sdhci_pltfm_priv(pltfm_host);
    struct rk35xx_priv *priv = dwc_priv->priv;
    rt_uint8_t txclk_tapnum = DLL_TXCLK_TAPNUM_DEFAULT;
    rt_uint32_t extra, reg;
    int err;

    host->mmc->actual_clock = 0;

    if (clock == 0){
        sdhci_set_clock(host, clock);
        return;
    }

    if (clock <= 400000)
        clock = 375000;

    err = rt_clk_set_rate(pltfm_host->clk, clock);
    if (err)
        dev_err(host->mmc->parent, "fail to set clock %d", clock);
    
    sdhci_set_clock(host, clock);

    reg = dwc_priv->vendor_specific_area1 + DWCMSHC_HOST_CTRL3;
    extra = sdhci_readl(host, reg);
    extra &= ~BIT(0);
    sdhci_writel(host, extra, reg);

    if (clock <= 52000000){
        sdhci_writel(host, DWCMSHC_EMMC_DLL_BYPASS | DWCMSHC_EMMC_DLL_START, DWCMSHC_EMMC_DLL_CTRL);
        sdhci_writel(host, DLL_RXCLK_ORI_GATE, DWCMSHC_EMMC_DLL_RXCLK);
        sdhci_writel(host, 0, DWCMSHC_EMMC_DLL_TXCLK);
        sdhci_writel(host, 0, DECMSHC_EMMC_DLL_CMDOUT);

        extra = DWCMSHC_EMMC_DLL_DLYENA |
			DLL_STRBIN_DELAY_NUM_SEL |
			DLL_STRBIN_DELAY_NUM_DEFAULT << DLL_STRBIN_DELAY_NUM_OFFSET;
        sdhci_writel(host, extra, DWCMSHC_EMMC_DLL_STRBIN);
        return;
    }

    sdhci_writel(host, BIT(1), DWCMSHC_EMMC_DLL_CTRL);
    udelay(1);
    sdhci_writel(host, 0x0, DWCMSHC_EMMC_DLL_CTRL);

    extra = DWCMSHC_EMMC_DLL_DLYENA;
    if (priv->devtype == DWCMSHC_RK3568)
        extra |= DLL_RXCLK_NO_INVERTER << DWCMSHC_EMMC_DLL_RXCLK_SRCSEL;
    sdhci_writel(host, extra, DWCMSHC_EMMC_DLL_RXCLK);

    extra = 0x5 << DWCMSHC_EMMC_DLL_START_POINT |
        0x2 << DWCMSHC_EMMC_DLL_INC |
        DWCMSHC_EMMC_DLL_START;
    sdhci_writel(host, extra, DWCMSHC_EMMC_DLL_CTRL);
    err = readl_poll_timeout(host->ioaddr + DWCMSHC_EMMC_DLL_STATUS0,
                    extra, DLL_LOCK_WO_TMOUT(extra), 1,
                    500 * USEC_PER_MSEC);
    if (err) {
        dev_err(mmc_dev(host->mmc), "DLL lock timeout!\n");
        return;
    }

    extra = 0x1 << 16 |
        0x3 << 17 |
        0x3 << 19;
    sdhci_writel(host, extra, dwc_priv->vendor_specific_area1 + DWCMSHC_EMMC_ATCTRL);

    if (host->mmc->ios.timing == MMC_TIMING_MMC_HS200 ||
        host->mmc->ios.timing == MMC_TIMING_MMC_HS400)
        txclk_tapnum = priv->txclk_tapnum;
    
    extra = DWCMSHC_EMMC_DLL_DLYENA |
        DLL_TXCLK_TAPNUM_FROM_SW |
        DLL_RXCLK_NO_INVERTER << DWCMSHC_EMMC_DLL_RXCLK_SRCSEL |
        txclk_tapnum;
    sdhci_writel(host, extra, DWCMSHC_EMMC_DLL_TXCLK);

    extra = DWCMSHC_EMMC_DLL_DLYENA |
        DLL_STRBIN_TAPNUM_DEFAULT |
        DLL_STRBIN_TAPNUM_FROM_SW;
    sdhci_writel(host, extra, DWCMSHC_EMMC_DLL_STRBIN);
}

static void dwcmshc_set_uhs_signaling(struct sdhci_host *host,
				      unsigned int timing)
{
    struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
    struct dwcmshc_priv *priv = sdhci_pltfm_priv(pltfm_host);
    rt_uint16_t ctrl, ctrl_2;

    ctrl_2 = sdhci_readw(host, SDHCI_HOST_CONTROL2);

    ctrl_2 &= ~SDHCI_CTRL_UHS_MASK;
    if ((timing == MMC_TIMING_MMC_HS200) ||
        (timing == MMC_TIMING_UHS_SDR104))
        ctrl_2 |= SDHCI_CTRL_UHS_SDR104;
    else if (timing == MMC_TIMING_UHS_SDR12)
        ctrl_2 |= SDHCI_CTRL_UHS_SDR12;
    else if ((timing == MMC_TIMING_UHS_SDR25) ||
        (timing == MMC_TIMING_MMC_HS))
        ctrl_2 |= SDHCI_CTRL_UHS_SDR25;
    else if (timing == MMC_TIMING_UHS_SDR50)
        ctrl_2 |= SDHCI_CTRL_UHS_SDR50;
    else if ((timing == MMC_TIMING_UHS_DDR50) ||
		(timing == MMC_TIMING_MMC_DDR52))
        ctrl_2 |= SDHCI_CTRL_UHS_DDR50;
    else if (timing == MMC_TIMING_MMC_HS400) {
        ctrl = sdhci_readw(host, priv->vendor_specific_area1 + DWCMSHC_EMMC_CONTROL);
        ctrl |= DWCMSHC_CARD_IS_EMMC;
        sdhci_writew(host, ctrl, priv->vendor_specific_area1 + DWCMSHC_EMMC_CONTROL);

        ctrl_2 |= DWCMSHC_CTRL_HS400;
    }

    sdhci_writew(host, ctrl_2, SDHCI_HOST_CONTROL2);  
}

static unsigned int rk35xx_get_max_clock(struct sdhci_host *host)
{
    struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);

    return rt_clk_round_rate(pltfm_host->clk, ULONG_MAX);
}

static void rk35xx_sdhci_reset(struct sdhci_host *host, rt_uint8_t mask)
{
    struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
    struct dwcmshc_priv *dwc_priv = sdhci_pltfm_priv(pltfm_host);
    struct rk35xx_priv *priv = dwc_priv->priv;

    if (mask & SDHCI_RESET_ALL && priv->reset) {
        rt_reset_control_assert(priv->reset);
        udelay(1);
        rt_reset_control_deassert(priv->reset);
    }

    sdhci_reset(host, mask);
}

static const struct sdhci_ops sdhci_dwcmshc_rk35xx_ops = {
    .set_clock         = dwcmshc_rk3568_set_clock,
    .set_bus_width     = sdhci_set_bus_width,
    .set_uhs_signaling = dwcmshc_set_uhs_signaling,
    .get_max_clock     = rk35xx_get_max_clock,
    .reset             = rk35xx_sdhci_reset,
};
static const struct sdhci_pltfm_data sdhci_dwcmshc_data =
{
};

static const struct sdhci_pltfm_data sdhci_dwcmshc_rk35xx_data =
{
    .ops     = &sdhci_dwcmshc_rk35xx_ops,
    .quirks  = SDHCI_QUIRK_CAP_CLOCK_BASE_BROKEN |
           SDHCI_QUIRK_BROKEN_TIMEOUT_VAL,
    .quirks2 = SDHCI_QUIRK2_PRESET_VALUE_BROKEN |
           SDHCI_QUIRK2_CLOCK_DIV_ZERO_BROKEN, 
};

static const struct rt_ofw_node_id sdhci_dwcmshc_ofw_ids[] =
{
    { .compatible = "rockchip,rk3588-dwcmshc", .data = &sdhci_dwcmshc_rk35xx_data, },
    { .compatible = "rockchip,rk3568-dwcmshc", .data = &sdhci_dwcmshc_rk35xx_data, },
    { .compatible = "rockchip,dwcmshc-sdhci", .data = &sdhci_dwcmshc_rk35xx_data, },
    { .compatible = "snps,dwcmshc-sdhci", .data = &sdhci_dwcmshc_data, },
    { /* sentinel */ }
};

static int dwcmshc_rk35xx_init(struct sdhci_host *host, struct dwcmshc_priv *dwc_priv)
{
    int err;
    struct rk35xx_priv *priv = dwc_priv->priv;
    priv->reset = rt_reset_control_get_array(host->mmc->parent);
    if (IS_ERR(priv->reset)){
        err = PTR_ERR(priv->reset);
        dev_err(host->mmc->parent, "failed to get reset control %d\n", err);
        return err;
    }

    priv->clk_arr = rt_clk_get_array(host->mmc->parent);

    err = rt_clk_array_prepare_enable(priv->clk_arr);

    if (err) {
        dev_err(mmc_dev(host->name), "failded to enable clocks %d\n", err);
        return err;
    }

    if (rt_ofw_prop_read_u8(host->mmc->parent->ofw_node, "rockchip,txclk-tapnum",
                &priv->txclk_tapnum))
        priv->txclk_tapnum = DLL_TXCLK_TAPNUM_DEFAULT;
    
    sdhci_writel(host, 0x0, dwc_priv->vendor_specific_area1 + DWCMSHC_HOST_CTRL3);

    sdhci_writel(host, 0, DWCMSHC_EMMC_DLL_TXCLK);
    sdhci_writel(host, 0, DWCMSHC_EMMC_DLL_STRBIN);

    return 0;
}

static void dwcmshc_rk35xx_postinit(struct sdhci_host *host, struct dwcmshc_priv *dwc_priv)
{
    return;
}

static rt_err_t sdhci_dwcmshc_probe(struct rt_platform_device *pdev)
{
    struct rt_device *dev = &pdev->parent;
    struct sdhci_pltfm_host *pltfm_host;
    struct sdhci_host *host;
    struct dwcmshc_priv *priv;
    struct rk35xx_priv *rk_priv = NULL;
    int err;

    host = sdhci_pltfm_init(pdev, &sdhci_dwcmshc_rk35xx_data, 
                sizeof(struct dwcmshc_priv));
    
    if (IS_ERR(host))
        return PTR_ERR(host);

    pltfm_host = sdhci_priv(host);
    priv = sdhci_pltfm_priv(pltfm_host);

    if (dev->ofw_node) {
        pltfm_host->clk = rt_clk_get_by_name(dev, "core");
        if (IS_ERR(pltfm_host->clk)){
            err = PTR_ERR(pltfm_host->clk);
            dev_err(dev, "failed to get core clk: %d\n", err);
            goto free_pltfm;
        }
        err = rt_clk_prepare_enable(pltfm_host->clk);
        if (err)
            goto free_pltfm;
        
        priv->bus_clk = rt_clk_get_by_name(dev, "bus");
        if (!IS_ERR(priv->bus_clk))
            rt_clk_prepare_enable(priv->bus_clk);
    }

    err = mmc_of_parse(host->mmc);
    if (err)
        goto err_clk;

    sdhci_get_of_property(pdev);

    priv->vendor_specific_area1 =
        sdhci_readl(host, DWCMSHC_P_VENDOR_AREA1) & DWCMSHC_AREA1_MASK;

    rk_priv = devm_kzalloc(dev, sizeof(struct rk35xx_priv), GFP_KERNEL);

    if (!rk_priv){
        err = -RT_ENOMEM;
        goto err_clk;
    }

    if (rt_ofw_node_is_compatible(dev->ofw_node, "rockchip,rk3588-dwcmshc"))
        rk_priv->devtype = DWCMSHC_RK3588;
    else
        rk_priv->devtype = DWCMSHC_RK3568;

    priv->priv = rk_priv;

    err = dwcmshc_rk35xx_init(host, priv);
    if (err)
        goto err_clk;

    //host->mmc->caps |= MMC_CAP_WAIT_WHILE_BUSY;

    err = sdhci_setup_host(host);
    if (err)
        goto err_rpm;

    if (rk_priv)
        dwcmshc_rk35xx_postinit(host, priv);

    err = __sdhci_add_host(host);

    if (err)
        goto err_setup_host;

    return RT_EOK;

err_setup_host:
    sdhci_cleanup_host(host);
err_rpm:
    dev_err(dev, "Error: No device match data found\n");
    return -RT_ERROR;
err_clk:
    rt_clk_disable_unprepare(pltfm_host->clk);
    rt_clk_disable_unprepare(priv->bus_clk);
    if (rk_priv)
        rt_clk_array_unprepare(rk_priv->clk_arr);

free_pltfm:
    sdhci_pltfm_free(pdev);
    return err;
}

static struct rt_platform_driver sdhci_dwcmshc_driver =
{
    .name = "sdhci-dwcmshc",
    .ids = sdhci_dwcmshc_ofw_ids,

    .probe = sdhci_dwcmshc_probe,
};
RT_PLATFORM_DRIVER_EXPORT(sdhci_dwcmshc_driver);
