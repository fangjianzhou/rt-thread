/*
 * Copyright (c) 2006-2023, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2023-10-31     Shell        init ver.
 */
#ifndef __TEST_LWP_COMMON_H__
#define __TEST_LWP_COMMON_H__

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <utest.h>

#include <board.h>
#include <rtthread.h>
#include <rthw.h>
#include <mmu.h>
#include <tlb.h>

#include <lwp_arch.h>

extern rt_base_t rt_heap_lock(void);
extern void rt_heap_unlock(rt_base_t level);

#define __int_compare(a, b, operator) do{long _a = (long)(a); long _b = (long)(b); __utest_assert((_a) operator (_b), "Assertion Failed: (" #a ") "#operator" (" #b ")"); if (!((_a) operator (_b)))LOG_E("\t"#a"=%ld(0x%lx), "#b"=%ld(0x%lx)", _a, _a, _b, _b);} while (0)
#define utest_int_equal(a, b) __int_compare(a, b, ==)
#define utest_int_not_equal(a, b) __int_compare(a, b, !=)
#define utest_int_less(a, b) __int_compare(a, b, <)
#define utest_int_less_equal(a, b) __int_compare(a, b, <=)

/**
 * @brief During the operations, is heap still the same;
 */
#define CONSIST_HEAP(statement) do {                \
    rt_size_t total, used, max_used;                \
    rt_size_t totala, useda, max_useda;             \
    rt_ubase_t level = rt_heap_lock();              \
    rt_memory_info(&total, &used, &max_used);       \
    statement;                                      \
    rt_memory_info(&totala, &useda, &max_useda);    \
    rt_heap_unlock(level);                          \
    utest_int_equal(total, totala);               \
    utest_int_equal(used, useda);                 \
    } while (0)


#endif /* __TEST_LWP_COMMON_H__ */
