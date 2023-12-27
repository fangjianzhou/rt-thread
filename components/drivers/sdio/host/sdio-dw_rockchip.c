/*
 * Copyright (c) 2006-2022, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-12-06     GuEe-GUI     first version
 */

#include "sdio-dw.h"
#include "sdio-dw-platfrom.h"

#define RK3288_CLKGEN_DIV 2

struct sdio_dw_rockchip_platfrom_data
{
    struct rt_clk *drv_clk;
    struct rt_clk *sample_clk;

    rt_uint32_t num_phases;
    rt_uint32_t default_sample_phase;
};

static rt_err_t sdio_dw_rockchip_probe(struct rt_platform_device *pdev)
{
    const struct sdio_dw_drv_data *drv_data = pdev->id->data;

    return sdio_dw_platfrom_register(pdev, drv_data);
}

static rt_err_t sdio_dw_rockchip_init(struct sdio_dw *sd)
{
    /* It is slot 8 on Rockchip SoCs */
    sd->sdio_id0 = 8;

    if (rt_ofw_node_is_compatible(sd->parent.ofw_node, "rockchip,rk3288-dw-mshc"))
    {
        rt_ubase_t rate;

        sd->bus_hz /= RK3288_CLKGEN_DIV;

        /*
         * clock driver will fail if the clock is less than the lowest source
         * clock divided by the internal clock divider. Test for the lowest
         * available clock and set the minimum freq to clock / clock divider.
         */
        rate = rt_clk_get_rate(sd->ciu_clk);

        if ((rt_base_t)rate > 0)
        {
            sd->minimum_speed = rate / RK3288_CLKGEN_DIV;
        }
    }

    return RT_EOK;
}

static rt_err_t sdio_dw_rk3288_set_iocfg(struct sdio_dw *sd, struct rt_mmcsd_io_cfg *ios)
{
    rt_uint32_t freq;
    rt_uint32_t bus_hz;

    if (!ios->clock)
    {
        return RT_EOK;
    }

    freq = ios->clock * RK3288_CLKGEN_DIV;

    /*
     * The clock frequency chosen here affects CLKDIV in the dw_mmc core.
     * That can be either 0 or 1, but it must be set to 1 for eMMC DDR52
     * 8-bit mode.  It will be set to 0 for all other modes.
     */
    if (ios->bus_width == MMCSD_BUS_WIDTH_8 && ios->timing == MMCSD_TIMING_MMC_DDR52)
    {
        freq *= 2;
    }

    rt_clk_set_rate(sd->ciu_clk, freq);
    bus_hz = rt_clk_get_rate(sd->ciu_clk) / RK3288_CLKGEN_DIV;

    if (bus_hz != sd->bus_hz)
    {
        sd->bus_hz = bus_hz;
        sd->current_speed = 0;
    }

    return RT_EOK;
}

static rt_err_t sdio_dw_rk3288_parse_ofw(struct sdio_dw *sd)
{
    rt_err_t err = RT_EOK;
    struct rt_ofw_node *np = sd->parent.ofw_node;
    struct sdio_dw_rockchip_platfrom_data *pdata = rt_malloc(sizeof(*pdata));

    if (!pdata)
    {
        return -RT_ENOMEM;
    }

    if (rt_ofw_prop_read_u32(np, "rockchip,desired-num-phases", &pdata->num_phases))
    {
        pdata->num_phases = 360;
    }

    if (rt_ofw_prop_read_u32(np, "rockchip,default-sample-phase", &pdata->default_sample_phase))
    {
        pdata->default_sample_phase = 0;
    }

    pdata->drv_clk = rt_ofw_get_clk_by_name(np, "ciu-drive");
    pdata->sample_clk = rt_ofw_get_clk_by_name(np, "ciu-sample");

    return err;
}

static const struct sdio_dw_drv_data rk2928_drv_data =
{
    .init = sdio_dw_rockchip_init,
};

static const struct sdio_dw_drv_data rk3288_drv_data =
{
    .init = sdio_dw_rockchip_init,
    .set_iocfg = sdio_dw_rk3288_set_iocfg,
    .parse_ofw = sdio_dw_rk3288_parse_ofw,
};

static const struct rt_ofw_node_id sdio_dw_rockchip_ofw_ids[] =
{
    { .compatible = "rockchip,rk2928-dw-mshc", .data = &rk2928_drv_data },
    { .compatible = "rockchip,rk3288-dw-mshc", .data = &rk3288_drv_data },
    { /* sentinel */ }
};

static struct rt_platform_driver sdio_dw_rockchip_driver =
{
    .name = "dw-mmc-rockchip",
    .ids = sdio_dw_rockchip_ofw_ids,

    .probe = sdio_dw_rockchip_probe,
};
RT_PLATFORM_DRIVER_EXPORT(sdio_dw_rockchip_driver);
