/*
 * Copyright (c) 2006-2023, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2023-10-31     Shell        test case for pid_get()
 */
#include "common.h"

#include <lwp.h>
#include <lwp_pid.h>
#include <rtthread.h>

static rt_lwp_t process[RT_LWP_MAX_NR];

static void pid_get_tc(void)
{
    long ref;
    rt_lwp_t lwp;

    /* take out all resources */
    for (size_t i = 0; i < RT_LWP_MAX_NR; i++)
    {
        lwp = lwp_create(LWP_CREATE_FLAG_ALLOC_PID);
        uassert_not_null(lwp);
        utest_int_not_equal(lwp->pid, 0);
        LOG_D("Allocated pid: %d", lwp->pid);
        process[i] = lwp;
    }

    /* test if failed */
    lwp = lwp_create(LWP_CREATE_FLAG_ALLOC_PID);
    uassert_null(lwp);

    /* take out all resources */
    for (size_t i = 0; i < RT_LWP_MAX_NR; i++)
    {
        lwp_pid_put(process[i]);
        ref = lwp_ref_dec(process[i]);
        utest_int_equal(ref, 1);

        process[i] = 0;
    }
}

static rt_err_t utest_tc_init(void)
{
    return RT_EOK;
}

static rt_err_t utest_tc_cleanup(void)
{
    return RT_EOK;
}

static void testcase(void)
{
    UTEST_UNIT_RUN(pid_get_tc);
}
UTEST_TC_EXPORT(testcase, "testcases.lwp.pid_get", utest_tc_init, utest_tc_cleanup, 10);
