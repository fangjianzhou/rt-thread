/*
 * Copyright (c) 2006-2023, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2023-09-23     GuEe-GUI     first version
 */

#define DBG_TAG "pci.dw.rockchip"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

#include "pcie-dw.h"

/*
 * The upper 16 bits of PCIE_CLIENT_CONFIG are a write
 * mask for the lower 16 bits.
 */
#define HIWORD_UPDATE(mask, val)        (((mask) << 16) | (val))
#define HIWORD_UPDATE_BIT(val)          HIWORD_UPDATE(val, val)
#define HIWORD_DISABLE_BIT(val)         HIWORD_UPDATE(val, ~val)

#define PCIE_CLIENT_RC_MODE             HIWORD_UPDATE_BIT(0x40)
#define PCIE_CLIENT_ENABLE_LTSSM        HIWORD_UPDATE_BIT(0xc)
#define PCIE_SMLH_LINKUP                RT_BIT(16)
#define PCIE_RDLH_LINKUP                RT_BIT(17)
#define PCIE_LINKUP                     (PCIE_SMLH_LINKUP | PCIE_RDLH_LINKUP)
#define PCIE_L0S_ENTRY                  0x11
#define PCIE_CLIENT_GENERAL_CONTROL     0x0
#define PCIE_CLIENT_INTR_STATUS_LEGACY  0x8
#define PCIE_CLIENT_INTR_MASK_LEGACY    0x1c
#define PCIE_CLIENT_GENERAL_DEBUG       0x104
#define PCIE_CLIENT_HOT_RESET_CTRL      0x180
#define PCIE_CLIENT_LTSSM_STATUS        0x300
#define PCIE_LTSSM_ENABLE_ENHANCE       RT_BIT(4)
#define PCIE_LTSSM_STATUS_MASK          RT_GENMASK(5, 0)

struct rockchip_pcie
{
    struct dw_pcie pci;
    void *apb_base;

    rt_ubase_t rst_pin;
    struct rt_phy_device *phy;
    struct rt_clk_array *clk_arr;
    struct rt_reset_control *rstc;
    struct rt_regulator *vpcie3v3;

    int intx_irq;
    struct rt_pic intx_pic;
};

static int rockchip_intx_irq_map(struct rt_pic *pic, int hwirq, rt_uint32_t mode)
{
    int irq;
    struct rt_pic_irq *pirq = rt_pic_find_irq(pic, hwirq);

    if (pirq)
    {
        if (pirq->irq >= 0)
        {
            irq = pirq->irq;
        }
        else
        {
            struct rockchip_pcie *rk_pcie = pic->priv_data;

            irq = rt_pic_config_irq(pic, hwirq, hwirq);
            rt_pic_cascade(pirq, rk_pcie->intx_irq);
            rt_pic_irq_set_triger_mode(irq, mode);
        }
    }
    else
    {
        irq = -1;
    }

    return irq;
}

static rt_err_t rockchip_intx_irq_parse(struct rt_pic *pic,
        struct rt_ofw_cell_args *args, struct rt_pic_irq *out_pirq)
{
    rt_err_t err = RT_EOK;

    if (args->args_count == 1)
    {
        out_pirq->hwirq = args->args[1];
    }
    else
    {
        err = -RT_EINVAL;
    }

    return err;
}

static struct rt_pic_ops rockchip_intx_ops =
{
    .name = "RK-INTx",
    .irq_ack = rt_pic_irq_parent_ack,
    .irq_mask = rt_pic_irq_parent_mask,
    .irq_unmask = rt_pic_irq_parent_unmask,
    .irq_eoi = rt_pic_irq_parent_eoi,
    .irq_set_priority = rt_pic_irq_parent_set_priority,
    .irq_set_affinity = rt_pic_irq_parent_set_affinity,
    .irq_set_triger_mode = rt_pic_irq_parent_set_triger_mode,
    .irq_map = rockchip_intx_irq_map,
    .irq_parse = rockchip_intx_irq_parse,
};

static rt_err_t rockchip_pcie_init_irq(struct rockchip_pcie *rk_pcie)
{
    struct rt_ofw_node *np = rk_pcie->pci.dev->ofw_node, *intx_np;
    struct rt_pic *intx_pic = &rk_pcie->intx_pic;

    intx_np = rt_ofw_get_child_by_tag(np, "legacy-interrupt-controller");

    if (!intx_np)
    {
        LOG_E("INTx ofw node not found");

        return -RT_EIO;
    }

    rk_pcie->intx_irq = rt_ofw_get_irq(intx_np, 0);

    rt_ofw_node_put(intx_np);

    if (rk_pcie->intx_irq < 0)
    {
        LOG_E("INTx irq get fail");

        return rk_pcie->intx_irq;
    }

    intx_pic->priv_data = rk_pcie;
    intx_pic->ops = &rockchip_intx_ops;

    rt_pic_linear_irq(intx_pic, RT_PCI_INTX_PIN_MAX);

    rt_pic_user_extends(intx_pic);

    rt_ofw_data(np) = intx_pic;

    return RT_EOK;
}

static void rockchip_pcie_remove(struct rockchip_pcie *rk_pcie)
{
    if (rk_pcie->vpcie3v3)
    {
        rt_regulator_disable(rk_pcie->vpcie3v3);
    }

    rt_free(rk_pcie);
}

static rt_err_t rockchip_pcie_probe(struct rt_platform_device *pdev)
{
    rt_err_t err;
    struct rt_device *dev = &pdev->parent;
    struct rockchip_pcie *rk_pcie = rt_calloc(1, sizeof(*rk_pcie));

    if (!rk_pcie)
    {
        return -RT_EINVAL;
    }

    rk_pcie->pci.dev = dev;
    rk_pcie->vpcie3v3 = rt_regulator_get_optional(dev, "vpcie3v3");

    if (rk_pcie->vpcie3v3)
    {
        if ((err = rt_regulator_enable(rk_pcie->vpcie3v3)))
        {
            goto _free_res;
        }
    }

    if ((err = rockchip_pcie_init_irq(rk_pcie)))
    {
        goto _free_res;
    }

	return RT_EOK;

_free_res:
    rockchip_pcie_remove(rk_pcie);

    return err;
}

static const struct rt_ofw_node_id rockchip_pcie_ofw_ids[] =
{
    { .compatible = "rockchip,rk3568-pcie" },
    { /* sentinel */ }
};

static struct rt_platform_driver rockchip_pcie_driver =
{
    .name = "rockchip-dw-pcie",
    .ids = rockchip_pcie_ofw_ids,

    .probe = rockchip_pcie_probe,
};
RT_PLATFORM_DRIVER_EXPORT(rockchip_pcie_driver);
