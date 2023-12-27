/*
 * Copyright (c) 2006-2022, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-11-26     GuEe-GUI     first version
 */

#include <rtthread.h>
#include <rtdevice.h>

#define DBG_TAG "mfd.rk8xx-i2c"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

#include "rk8xx.h"

struct rk8xx_i2c_soc_data
{
    int variant;
};

static rt_uint32_t rk8xx_i2c_read(struct rk8xx *rk8xx, rt_uint16_t reg)
{
    rt_uint8_t data = 0;
    rt_uint8_t send_buf[2];
    struct rt_i2c_msg msg[2];
    struct rt_i2c_client *client = rk8xx_to_i2c_client(rk8xx);

    send_buf[0] = (reg & 0xff);

    msg[0].addr  = client->client_addr;
    msg[0].flags = RT_I2C_WR;
    msg[0].len   = 1;
    msg[0].buf   = send_buf;

    msg[1].addr  = client->client_addr;
    msg[1].flags = RT_I2C_RD;
    msg[1].len   = 1;
    msg[1].buf   = &data;

    if (rt_i2c_transfer(client->bus, msg, 2) == 2)
    {
        return data;
    }
    else
    {
        return (rt_uint32_t)-RT_ERROR;
    }
}

static rt_err_t rk8xx_i2c_write(struct rk8xx *rk8xx, rt_uint16_t reg,
        rt_uint8_t data)
{
    rt_uint8_t send_buf[2];
    struct rt_i2c_msg msg;
    struct rt_i2c_client *client = rk8xx_to_i2c_client(rk8xx);

    send_buf[0] = reg & 0xff;
    send_buf[1] = data;

    msg.addr = client->client_addr;
    msg.flags = RT_I2C_WR;
    msg.len = 2;
    msg.buf = send_buf;

    if (rt_i2c_transfer(client->bus, &msg, 1) == 1)
    {
        return RT_EOK;
    }
    else
    {
        return -RT_ERROR;
    }
}

static rt_err_t rk8xx_i2c_update_bits(struct rk8xx *rk8xx, rt_uint16_t reg,
        rt_uint8_t mask, rt_uint8_t data)
{
    rt_uint32_t old, tmp;

    old = rk8xx_i2c_read(rk8xx, reg);

    if (old < 0)
    {
        return old;
    }

    tmp = old & ~mask;
    tmp |= (data & mask);

    return rk8xx_i2c_write(rk8xx, reg, tmp);
}

static rt_err_t create_rk8xx_i2c_platform_device(rt_bus_t platform_bus,
        struct rt_i2c_client *client,
        const struct rk8xx_i2c_soc_data *soc_data,
        const char *name,
        int irq,
        void *priv)
{
    rt_err_t err;
    struct rk8xx *rk8xx;
    struct rt_platform_device *pdev = rt_platform_device_alloc(name);

    if (!pdev)
    {
        return -RT_ENOMEM;
    }

    rk8xx = rt_malloc(sizeof(*rk8xx));

    if (!rk8xx)
    {
        rt_free(pdev);

        return -RT_ENOMEM;
    }

    rk8xx->variant = soc_data->variant;
    rk8xx->irq = irq;
    rk8xx->dev = &client->parent;
    rk8xx->read = rk8xx_i2c_read;
    rk8xx->write = rk8xx_i2c_write;
    rk8xx->update_bits = rk8xx_i2c_update_bits;
    rk8xx->priv = priv;

    pdev->priv = rk8xx;

    err = rt_bus_add_device(platform_bus, &pdev->parent);

    if (err && err != -RT_ENOSYS)
    {
        LOG_E("Add RK8XX - %s error = %s", name, rt_strerror(err));
    }

    return err;
}

static rt_err_t rk8xx_i2c_probe(struct rt_i2c_client *client)
{
    int irq;
    rt_err_t err;
    struct rt_device *dev;
    struct rt_ofw_node *np, *client_np;
    const struct rk8xx_i2c_soc_data *soc_data;
    rt_bus_t platform_bus = rt_bus_find_by_name("platform");

    if (!platform_bus)
    {
        return -RT_EIO;
    }

    irq = rt_dm_dev_get_irq(&client->parent, 0);

    if (irq < 0)
    {
        return irq;
    }

    dev = &client->parent;
    client_np = dev->ofw_node;
    soc_data = client->ofw_id->data;

    rt_pin_ctrl_confs_apply_by_name(dev, RT_NULL);

    create_rk8xx_i2c_platform_device(platform_bus, client, soc_data,
            "rk8xx-rtc", irq, RT_NULL);

    if ((np = rt_ofw_get_child_by_tag(client_np, "regulators")))
    {
        rt_ofw_node_put(np);

        err = create_rk8xx_i2c_platform_device(platform_bus, client, soc_data,
                "rk8xx-regulator", irq, np);

        if (err == -RT_ENOMEM)
        {
            return err;
        }
    }

    return RT_EOK;
}

static const struct rk8xx_i2c_soc_data rk805_data =
{
    .variant = RK805_ID,
};

static const struct rk8xx_i2c_soc_data rk808_data =
{
    .variant = RK808_ID,
};

static const struct rk8xx_i2c_soc_data rk809_data =
{
    .variant = RK809_ID,
};

static const struct rk8xx_i2c_soc_data rk817_data =
{
    .variant = RK817_ID,
};

static const struct rk8xx_i2c_soc_data rk818_data =
{
    .variant = RK818_ID,
};

static const struct rt_ofw_node_id rk8xx_i2c_ofw_ids[] =
{
    { .compatible = "rockchip,rk805", .data = &rk805_data },
    { .compatible = "rockchip,rk808", .data = &rk808_data },
    { .compatible = "rockchip,rk809", .data = &rk809_data },
    { .compatible = "rockchip,rk817", .data = &rk817_data },
    { .compatible = "rockchip,rk818", .data = &rk818_data },
    { /* sentinel */ },
};

static struct rt_i2c_driver rk8xx_i2c_driver =
{
    .ofw_ids = rk8xx_i2c_ofw_ids,

    .probe = rk8xx_i2c_probe,
};
RT_I2C_DRIVER_EXPORT(rk8xx_i2c_driver);
