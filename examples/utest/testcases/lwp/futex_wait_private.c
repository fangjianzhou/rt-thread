/*
 * Copyright (c) 2006-2023, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2023-11-02     Shell        test case for futex(2)
 */
#include "common.h"
#include "lwp_user_mm.h"
#include "mmu.h"
#include "utest_assert.h"

#include <lwp.h>
#include <lwp_pid.h>
#include <rtthread.h>

static rt_lwp_t lwp;
static int *uaddr = (int *)(0x200000 + 0x4800);
static const long wait_sec = 0;
static const long wait_nsec = 100;

static void futex_wait_private_tc(void)
{
    struct timespec to = {
        .tv_nsec = wait_nsec,
        .tv_sec = wait_sec,
    };

    utest_int_equal(
        lwp_futex(lwp, uaddr, FUTEX_WAIT | FUTEX_PRIVATE, 1, &to, RT_NULL, 0),
        ETIMEDOUT
    );
}

static rt_err_t utest_tc_init(void)
{
    lwp = lwp_create(LWP_CREATE_FLAG_ALLOC_PID | LWP_CREATE_FLAG_INIT_USPACE);
    uassert_not_null(lwp);

    utest_int_equal(
        lwp_map_user(lwp, uaddr, sizeof(int), 0),
        uaddr
    );
    rt_hw_aspace_switch(lwp->aspace);
    return RT_EOK;
}

static rt_err_t utest_tc_cleanup(void)
{
    utest_int_equal(lwp_ref_dec(lwp), 1);
    rt_hw_aspace_switch(0);

    return RT_EOK;
}

static void testcase(void)
{
    UTEST_UNIT_RUN(futex_wait_private_tc);
}
UTEST_TC_EXPORT(testcase, "testcases.lwp.futex_wait_private", utest_tc_init, utest_tc_cleanup, 10);
