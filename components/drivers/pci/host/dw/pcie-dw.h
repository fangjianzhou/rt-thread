/*
 * Copyright (c) 2006-2023, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2023-09-23     GuEe-GUI     first version
 */

#ifndef __PCIE_DESIGNWARE_H__
#define __PCIE_DESIGNWARE_H__

#include <rtthread.h>
#include <rtdevice.h>

struct dw_pcie_ops;

struct dw_pcie
{
    struct rt_device *dev;

    void *dbi_base;
    void *dbi_base2;
    void *atu_base;

    rt_size_t atu_size;
    rt_uint32_t num_ib_windows;
    rt_uint32_t num_ob_windows;
    rt_uint32_t region_align;
    rt_uint64_t region_limit;

    const struct dw_pcie_ops *ops;
};

struct dw_pcie_ops
{
    rt_uint64_t (*cpu_addr_fixup)(struct dw_pcie *pcie, rt_uint64_t cpu_addr);
    rt_uint32_t (*read_dbi)(struct dw_pcie *pcie, void *base, rt_uint32_t reg, rt_size_t size);
    void        (*write_dbi)(struct dw_pcie *pcie, void *base, rt_uint32_t reg, rt_size_t size, rt_uint32_t val);
    void        (*write_dbi2)(struct dw_pcie *pcie, void *base, rt_uint32_t reg, rt_size_t size, rt_uint32_t val);
    rt_err_t    (*link_up)(struct dw_pcie *pcie);
    rt_err_t    (*start_link)(struct dw_pcie *pcie);
    void        (*stop_link)(struct dw_pcie *pcie);
};

#endif /* __PCIE_DESIGNWARE_H__ */
