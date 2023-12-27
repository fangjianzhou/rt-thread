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

#define DBG_TAG "mfd.rk8xx-spi"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

#include "rk8xx.h"

#define RK806_ADDR_SIZE 2
#define RK806_CMD_WITH_SIZE(CMD, VALUE_BYTES) \
        (RK806_CMD_##CMD | RK806_CMD_CRC_DIS | (VALUE_BYTES - 1))

static rt_uint32_t rk8xx_spi_read(struct rk8xx *rk8xx, rt_uint16_t reg)
{
    rt_err_t err;
    rt_uint32_t data = 0;
    rt_uint8_t txbuf[3] = { 0 };
    struct rt_spi_device *spi_dev = rk8xx_to_spi_device(rk8xx);

    /* TX buffer contains command byte followed by two address bytes */
    txbuf[0] = RK806_CMD_WITH_SIZE(READ, sizeof(data));
    rt_memcpy(&txbuf[0], &reg, sizeof(reg));

    err = rt_spi_send_then_recv(spi_dev, txbuf, sizeof(txbuf), &data, sizeof(data));

    if (!err)
    {
        return data;
    }
    else
    {
        return (rt_uint32_t)err;
    }
}

static rt_err_t rk8xx_spi_write(struct rk8xx *rk8xx, rt_uint16_t reg,
        rt_uint8_t data)
{
    rt_uint8_t cmd;
    struct rt_spi_message xfer[2] = { };
    struct rt_spi_device *spi_dev = rk8xx_to_spi_device(rk8xx);

    cmd = RK806_CMD_WITH_SIZE(WRITE, sizeof(data));

    xfer[0].send_buf = &cmd;
    xfer[0].length = sizeof(cmd);
    xfer[1].send_buf = &data;
    xfer[1].length = sizeof(data);
    xfer[1].next = &xfer[0];

    return rt_spi_transfer_message(spi_dev, xfer) ? RT_EOK : -RT_EIO;
}

static rt_err_t rk8xx_spi_update_bits(struct rk8xx *rk8xx, rt_uint16_t reg,
        rt_uint8_t mask, rt_uint8_t data)
{
    rt_uint32_t old, tmp;

    old = rk8xx_spi_read(rk8xx, reg);

    if (old < 0)
    {
        return old;
    }

    tmp = old & ~mask;
    tmp |= (data & mask);

    return rk8xx_spi_write(rk8xx, reg, tmp);
}

static rt_err_t create_rk8xx_spi_platform_device(rt_bus_t platform_bus,
        struct rt_spi_device *spi_dev,
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

    rk8xx->variant = RK806_ID;
    rk8xx->irq = irq;
    rk8xx->dev = &spi_dev->parent;
    rk8xx->read = rk8xx_spi_read;
    rk8xx->write = rk8xx_spi_write;
    rk8xx->update_bits = rk8xx_spi_update_bits;
    rk8xx->priv = priv;

    pdev->priv = rk8xx;

    err = rt_bus_add_device(platform_bus, &pdev->parent);

    if (err && err != -RT_ENOSYS)
    {
        LOG_E("Add RK8XX - %s error = %s", name, rt_strerror(err));
    }

    return err;
}

static rt_err_t rk8xx_spi_probe(struct rt_spi_device *spi_dev)
{
    int irq;
    rt_err_t err;
    struct rt_device *dev;
    struct rt_ofw_node *np, *spi_dev_np;
    rt_bus_t platform_bus = rt_bus_find_by_name("platform");

    if (!platform_bus)
    {
        return -RT_EIO;
    }

    irq = rt_dm_dev_get_irq(&spi_dev->parent, 0);

    if (irq < 0)
    {
        return irq;
    }

    dev = &spi_dev->parent;
    spi_dev_np = dev->ofw_node;

    rt_pin_ctrl_confs_apply_by_name(dev, RT_NULL);

    if ((np = rt_ofw_get_child_by_tag(spi_dev_np, "regulators")))
    {
        rt_ofw_node_put(np);

        err = create_rk8xx_spi_platform_device(platform_bus, spi_dev,
                "rk8xx-regulator", irq, np);

        if (err == -RT_ENOMEM)
        {
            return err;
        }
    }

    return RT_EOK;
}

static const struct rt_spi_device_id rk8xx_spi_ids[] =
{
    { .name = "rk806" },
    { /* sentinel */ },
};

static const struct rt_ofw_node_id rk8xx_spi_ofw_ids[] =
{
    { .compatible = "rockchip,rk806" },
    { /* sentinel */ },
};

static struct rt_spi_driver rk8xx_spi_driver =
{
    .ids = rk8xx_spi_ids,
    .ofw_ids = rk8xx_spi_ofw_ids,

    .probe = rk8xx_spi_probe,
};
RT_SPI_DRIVER_EXPORT(rk8xx_spi_driver);
