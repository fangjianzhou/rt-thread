/*
 * Copyright (c) 2006-2023, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2023-11-30     Shell        Add itimer support
 */

#define DBG_TAG "lwp.signal"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

#define _GNU_SOURCE
#include <sys/time.h>
#undef _GNU_SOURCE

#include <rthw.h>
#include <rtthread.h>
#include <string.h>

#include "lwp_internal.h"
#include "sys/signal.h"
#include "syscall_generic.h"

rt_err_t lwp_signal_setitimer(rt_lwp_t lwp, int which, const struct itimerspec *restrict new, struct itimerspec *restrict old)
{
    int rc = 0;
    timer_t timerid = 0;
    int flags = 0;

    switch (which)
    {
        case ITIMER_REAL:
            timerid = lwp->signal.real_timer;
            rc = timer_settime(timerid, flags, new, old);
        default:
            rc = -ENOSYS;
            break;
    }

    return rc;
}
