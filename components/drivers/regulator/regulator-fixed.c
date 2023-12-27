/*
 * Copyright (c) 2006-2023, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2023-09-23     GuEe-GUI     first version
 */

#include "regulator_dm.h"

struct regulator_fixed
{
    struct rt_regulator_node parent;

    const char *input_supply;
    rt_uint32_t startup_delay;
    rt_uint32_t off_on_delay;
    struct rt_regulator_param param;
};

#define raw_to_regulator_fixed(raw) rt_container_of(raw, struct regulator_fixed, parent)

static int regulator_fixed_get_voltage(struct rt_regulator_node *reg_np)
{
    struct regulator_fixed *rf = raw_to_regulator_fixed(reg_np);

    return rf->param.min_uvolt + (rf->param.max_uvolt - rf->param.min_uvolt) / 2;
}

static const struct rt_regulator_ops regulator_fixed_ops =
{
    .get_voltage = regulator_fixed_get_voltage,
};

static rt_err_t regulator_fixed_probe(struct rt_platform_device *pdev)
{
    rt_err_t err;
    struct rt_device *dev = &pdev->parent;
    struct regulator_fixed *rf = rt_malloc(sizeof(*rf));
    struct rt_regulator_node *rnp;

    if (!rf)
    {
        return -RT_ENOMEM;
    }

    regulator_ofw_parse(dev->ofw_node, &rf->param);

    rnp = &rf->parent;
    rnp->supply_name = rf->param.name;
    rnp->ops = &regulator_fixed_ops;
    rnp->param = &rf->param;
    rnp->dev = &pdev->parent;

    rt_dm_dev_prop_read_u32(dev, "startup-delay-us", &rf->startup_delay);
    rt_dm_dev_prop_read_u32(dev, "off-on-delay-us", &rf->off_on_delay);

    if (rt_dm_dev_prop_read_bool(dev, "vin-supply"))
    {
        rf->input_supply = "vin";
    }

    if ((err = rt_regulator_register(rnp)))
    {
        rt_free(rf);
    }

    return RT_EOK;
}

static const struct rt_ofw_node_id regulator_fixed_ofw_ids[] =
{
    { .compatible = "regulator-fixed" },
    { /* sentinel */ }
};

static struct rt_platform_driver regulator_fixed_driver =
{
    .name = "reg-fixed-voltage",
    .ids = regulator_fixed_ofw_ids,

    .probe = regulator_fixed_probe,
};

static int regulator_fixed_register(void)
{
    rt_platform_driver_register(&regulator_fixed_driver);

    return 0;
}
INIT_SUBSYS_EXPORT(regulator_fixed_register);
