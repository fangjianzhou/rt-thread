/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Wait for bit with timeout and ctrlc
 *
 * (C) Copyright 2015 Mateusz Kulikowski <mateusz.kulikowski@gmail.com>
 */

#ifndef __WAIT_BIT_H
#define __WAIT_BIT_H

#include <rtthread.h>
#include "phy/phy.h" // for us_delay
#include "dwc_eth_qos.h" // for readl
/**
 * wait_for_bit_x()	waits for bit set/cleared in register
 *
 * Function polls register waiting for specific bit(s) change
 * (either 0->1 or 1->0). It can fail under two conditions:
 * - Timeout
 * - User interaction (CTRL-C)
 * Function succeeds only if all bits of masked register are set/cleared
 * (depending on set option).
 *
 * @param reg		Register that will be read (using read_x())
 * @param mask		Bit(s) of register that must be active
 * @param set		Selects wait condition (bit set or clear)
 * @param timeout_ms	Timeout (in milliseconds)
 * @param breakable	Enables CTRL-C interruption
 * Return:		0 on success, -ETIMEDOUT or -EINTR on failure
 */

#define BUILD_WAIT_FOR_BIT(sfx, type, read)				\
									\
static inline int wait_for_bit_##sfx(const void *reg,			\
				     const type mask,			\
				     const rt_bool_t set,			\
				     const rt_uint32_t timeout_ms)	\
{									\
	type val;							\
	rt_ubase_t start = rt_tick_get();				\
									\
	while (1) {							\
		val = read(reg);					\
									\
		if (!set)						\
			val = ~val;					\
									\
		if ((val & mask) == mask)				\
			return 0;					\
									\
		if (rt_tick_get() - start > rt_tick_from_millisecond(timeout_ms))   \
			break;						\
									\
		rt_thread_mdelay(1);						\
		rt_schedule();					\
	}								\
									\
/*	LOG_D("%s: Timeout (reg=%p mask=%x wait_set=%i)\n", __func__,*/	\
/*	      reg, mask, set);*/						\
									\
	return -RT_ETIMEOUT;						\
}

BUILD_WAIT_FOR_BIT(le32, rt_uint32_t, readl)

#endif