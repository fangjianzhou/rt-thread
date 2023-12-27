/*
 * Copyright (c) 2006-2022, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2023-11-03     zmshahaha    the first version
 */

#include <rtdevice.h>
#include "clk-rk.h"

#define PLL_MODE_MASK				0x3
#define PLL_RK3328_MODE_MASK			0x1

/* Define pll mode */
#define RKCLK_PLL_MODE_SLOW     0
#define RKCLK_PLL_MODE_NORMAL   1
#define RKCLK_PLL_MODE_DEEP     2

/* Only support RK3036 type CLK */
#define RK3036_PLLCON0_FBDIV_MASK      0xfff
#define RK3036_PLLCON0_FBDIV_SHIFT     0
#define RK3036_PLLCON0_POSTDIV1_MASK   (0x7 << 12)
#define RK3036_PLLCON0_POSTDIV1_SHIFT  12
#define RK3036_PLLCON1_LOCK_STATUS     (1 << 10)
#define RK3036_PLLCON1_REFDIV_MASK     0x3f
#define RK3036_PLLCON1_REFDIV_SHIFT    0
#define RK3036_PLLCON1_POSTDIV2_MASK   (0x7 << 6)
#define RK3036_PLLCON1_POSTDIV2_SHIFT  6
#define RK3036_PLLCON1_DSMPD_MASK      (0x1 << 12)
#define RK3036_PLLCON1_DSMPD_SHIFT     12
#define RK3036_PLLCON2_FRAC_MASK       0xffffff
#define RK3036_PLLCON2_FRAC_SHIFT      0
#define RK3036_PLLCON1_PWRDOWN_SHIT    13
#define RK3036_PLLCON1_PWRDOWN         (1 << RK3036_PLLCON1_PWRDOWN_SHIT)

#define VCO_MAX_HZ		(3200UL * MHZ)
#define VCO_MIN_HZ		(800UL * MHZ)
#define OUTPUT_MAX_HZ		(3200UL * MHZ)
#define OUTPUT_MIN_HZ		(24UL * MHZ)
#define MIN_FOUTVCO_FREQ	(800UL * MHZ)
#define MAX_FOUTVCO_FREQ	(2000UL * MHZ)

#define RK3588_VCO_MIN_HZ	(2250UL * MHZ)
#define RK3588_VCO_MAX_HZ	(4500UL * MHZ)
#define RK3588_FOUT_MIN_HZ	(37UL * MHZ)
#define RK3588_FOUT_MAX_HZ	(4500UL * MHZ)

#define RK3588_PLLCON(i)		((i) * 0x4)
#define RK3588_PLLCON0_M_MASK		(0x3ff << 0)
#define RK3588_PLLCON0_M_SHIFT		0
#define RK3588_PLLCON1_P_MASK		(0x3f << 0)
#define RK3588_PLLCON1_P_SHIFT		0
#define RK3588_PLLCON1_S_MASK		(0x7 << 6)
#define RK3588_PLLCON1_S_SHIFT		6
#define RK3588_PLLCON2_K_MASK		0xffff
#define RK3588_PLLCON2_K_SHIFT		0
#define RK3588_PLLCON1_PWRDOWN		(1 << 13)
#define RK3588_PLLCON6_LOCK_STATUS	(1 << 15)
#define RK3588_B0PLL_CLKSEL_CON(i)	((i) * 0x4 + 0x50000 + 0x300)
#define RK3588_B1PLL_CLKSEL_CON(i)	((i) * 0x4 + 0x52000 + 0x300)
#define RK3588_LPLL_CLKSEL_CON(i)	((i) * 0x4 + 0x58000 + 0x300)
#define RK3588_CORE_DIV_MASK		0x1f
#define RK3588_CORE_L02_DIV_SHIFT	0
#define RK3588_CORE_L13_DIV_SHIFT	7
#define RK3588_CORE_B02_DIV_SHIFT	8
#define RK3588_CORE_B13_DIV_SHIFT	0

void rational_best_approximation(rt_ubase_t given_numerator,
                    rt_ubase_t given_denominator,
                    rt_ubase_t max_numerator,
                    rt_ubase_t max_denominator,
                    rt_ubase_t *best_numerator,
                    rt_ubase_t *best_denominator);
rt_ubase_t rk_pll_get_rate(struct rk_pll_clock *pll, void *base, rt_ubase_t pll_id);
int rk_pll_set_rate(struct rk_pll_clock *pll, void *base, rt_ubase_t pll_id, rt_ubase_t drate);
const struct rk_cpu_rate_table *rk_get_cpu_settings(struct rk_cpu_rate_table *cpu_table, rt_ubase_t rate);
void rk_clk_set_default_rates(struct rt_clk *clk, rt_err_t (*clk_set_rate)(struct rt_clk *, rt_ubase_t, rt_ubase_t), int id);

