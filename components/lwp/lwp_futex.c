/*
 * Copyright (c) 2006-2023, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2021/01/02     bernard      the first version
 * 2023-07-25     Shell        Remove usage of rt_hw_interrupt API in the lwp
 *                             Coding style: remove multiple `return` in a routine
 * 2023-08-08     Shell        Fix return value of futex(wait); Fix ops that only
 *                             FUTEX_PRIVATE is supported currently
 * 2023-11-03     Shell        Add Support for ~FUTEX_PRIVATE
 */

#include "lwp_futex_internal.h"
#include "sys/time.h"

struct rt_mutex _glob_futex;

rt_err_t lwp_futex_init(void)
{
    return rt_mutex_init(&_glob_futex, "glob_ftx", RT_IPC_FLAG_PRIO);
}

static void _futex_lock(rt_lwp_t lwp, int op_flags)
{
    rt_err_t error;
    if (op_flags & FUTEX_PRIVATE)
    {
        LWP_LOCK(lwp);
    }
    else
    {
        error = lwp_mutex_take_safe(&_glob_futex, RT_WAITING_FOREVER, 0);
        if (error)
        {
            LOG_E("%s: Should not failed", __func__);
            RT_ASSERT(0);
        }
    }
}

static void _futex_unlock(rt_lwp_t lwp, int op_flags)
{
    rt_err_t error;
    if (op_flags & FUTEX_PRIVATE)
    {
        LWP_UNLOCK(lwp);
    }
    else
    {
        error = lwp_mutex_release_safe(&_glob_futex);
        if (error)
        {
            LOG_E("%s: Should not failed", __func__);
            RT_ASSERT(0);
        }
    }
}

/**
 * Destroy a Private FuTeX (pftx)
 * Note: must have futex address_search_head taken
 */
static rt_err_t _pftx_destroy_locked(void *data)
{
    rt_err_t ret = -1;
    rt_futex_t futex = (rt_futex_t)data;

    if (futex)
    {
        /**
         * Brief: Delete the futex from lwp address_search_head
         *
         * Note: Critical Section
         * - the lwp (READ. share by thread)
         * - the lwp address_search_head (RW. protected by caller. for destroy
         *   routine, it's always safe because it has already taken a write lock
         *    to the lwp.)
         */
        lwp_avl_remove(&futex->node, (struct lwp_avl_struct **)futex->node.data);

        /* release object */
        rt_free(futex);
        ret = 0;
    }
    return ret;
}

/**
 * Create a Private FuTeX (pftx)
 * Note: must have futex address_search_head taken
 */
static rt_futex_t _pftx_create_locked(int *uaddr, struct rt_lwp *lwp)
{
    rt_futex_t futex = RT_NULL;
    struct rt_object *obj = RT_NULL;

    /**
     * Brief: Create a futex under current lwp
     *
     * Note: Critical Section
     * - lwp (READ; share with thread)
     */
    if (lwp)
    {
        futex = (rt_futex_t)rt_malloc(sizeof(struct rt_futex));
        if (futex)
        {
            /* Create a Private FuTeX (pftx) */
            obj = rt_custom_object_create("pftx", (void *)futex, _pftx_destroy_locked);
            if (!obj)
            {
                rt_free(futex);
                futex = RT_NULL;
            }
            else
            {
                /**
                 * Brief: Add futex to user object tree for resource recycling
                 *
                 * Note: Critical Section
                 * - lwp user object tree (RW; protected by API)
                 * - futex (if the adding is successful, others can find the
                 *   unready futex. However, only the lwp_free will do this,
                 *   and this is protected by the ref taken by the lwp thread
                 *   that the lwp_free will never execute at the same time)
                 */
                if (lwp_user_object_add(lwp, obj))
                {
                    /* this will call a _pftx_destroy_locked, but that's okay */
                    rt_object_delete(obj);
                    rt_free(futex);
                    futex = RT_NULL;
                }
                else
                {
                    futex->node.avl_key = (avl_key_t)uaddr;
                    futex->node.data = &lwp->address_search_head;
                    futex->custom_obj = obj;
                    rt_list_init(&(futex->waiting_thread));

                    /**
                     * Brief: Insert into futex head
                     *
                     * Note: Critical Section
                     * - lwp address_search_head (RW; protected by caller)
                     */
                    lwp_avl_insert(&futex->node, &lwp->address_search_head);
                }
            }
        }
    }
    return futex;
}

/**
 * Get a Private FuTeX (pftx) match the (lwp, uaddr, op)
 */
static rt_futex_t _pftx_get(void *uaddr, struct rt_lwp *lwp, int op, rt_err_t *rc)
{
    struct lwp_avl_struct *node = RT_NULL;
    rt_futex_t futex = RT_NULL;
    rt_err_t error = -1;

    LWP_LOCK(lwp);

    /**
     * Note: Critical Section
     * protect lwp address_search_head (READ)
     */
    node = lwp_avl_find((avl_key_t)uaddr, lwp->address_search_head);
    if (node)
    {
        futex = rt_container_of(node, struct rt_futex, node);
        error = 0;
    }
    else
    {
        /* create a futex according to this uaddr */
        futex = _pftx_create_locked(uaddr, lwp);

        if (!futex)
            error = -ENOMEM;
        else
            error = 0;
    }
    LWP_UNLOCK(lwp);

    *rc = error;
    return futex;
}

/**
 * Destroy a Shared FuTeX (pftx)
 * Note: must have futex address_search_head taken
 */
static rt_err_t _sftx_destroy(void *data)
{
    rt_err_t ret = -1;
    rt_futex_t futex = (rt_futex_t )data;

    if (futex)
    {
        /* delete it even it's not in the table */
        futex_global_table_delete(&futex->entry.key);
        rt_free(futex);
        ret = 0;
    }
    return ret;
}

/**
 * Create a Shared FuTeX (sftx)
 */
static rt_futex_t _sftx_create(struct shared_futex_key *key, struct rt_lwp *lwp)
{
    rt_futex_t futex = RT_NULL;
    struct rt_object *obj = RT_NULL;

    if (lwp)
    {
        futex = (rt_futex_t)rt_calloc(1, sizeof(struct rt_futex));
        if (futex)
        {
            /* create a Shared FuTeX (sftx) */
            obj = rt_custom_object_create("sftx", (void *)futex, _sftx_destroy);
            if (!obj)
            {
                rt_free(futex);
                futex = RT_NULL;
            }
            else
            {
                if (futex_global_table_add(key, futex))
                {
                    rt_object_delete(obj);
                    rt_free(futex);
                    futex = RT_NULL;
                }
                else
                {
                    rt_list_init(&(futex->waiting_thread));
                    futex->custom_obj = obj;
                }
            }
        }
    }
    return futex;
}

/**
 * Get a Shared FuTeX (sftx) match the (lwp, uaddr, op)
 */
static rt_futex_t _sftx_get(void *uaddr, struct rt_lwp *lwp, int op, rt_err_t *rc)
{
    rt_futex_t futex = RT_NULL;
    struct shared_futex_key key;
    rt_varea_t varea;
    rt_err_t error = -1;

    RD_LOCK(lwp->aspace);
    varea = rt_aspace_query(lwp->aspace, uaddr);
    if (varea)
    {
        key.mobj = varea->mem_obj;
        key.offset = varea->offset;
        RD_UNLOCK(lwp->aspace);

        /* query for the key */
        error = futex_global_table_find(&key, &futex);
        if (error != RT_EOK)
        {
            /* not found, do allocation */
            futex = _sftx_create(&key, lwp);
            if (!futex)
                error = -ENOMEM;
            else
                error = 0;
        }
    }
    else
    {
        RD_UNLOCK(lwp->aspace);
    }

    *rc = error;
    return futex;
}

/* must have futex address_search_head taken */
static rt_futex_t _futex_get(void *uaddr, struct rt_lwp *lwp, int op_flags, rt_err_t *rc)
{
    rt_futex_t futex = RT_NULL;

    if (op_flags & FUTEX_PRIVATE)
    {
        futex = _pftx_get(uaddr, lwp, op_flags, rc);
    }
    else
    {
        futex = _sftx_get(uaddr, lwp, op_flags, rc);
    }

    return futex;
}

static rt_err_t _suspend_thread_timeout_locked(rt_thread_t thread, rt_futex_t futex, rt_tick_t timeout)
{
    rt_err_t rc;
    rc = rt_thread_suspend_with_flag(thread, RT_INTERRUPTIBLE);

    if (rc == RT_EOK)
    {
        /**
         * Brief: Add current thread into futex waiting thread list
         *
         * Note: Critical Section
         * - the futex waiting_thread list (RW)
         */
        rt_list_insert_before(&(futex->waiting_thread), &(thread->tlist));

        /* start the timer of thread */
        rt_timer_control(&(thread->thread_timer),
                            RT_TIMER_CTRL_SET_TIME,
                            &timeout);
        rt_timer_start(&(thread->thread_timer));
        rt_set_errno(ETIMEDOUT);
    }

    return rc;
}

static rt_err_t _suspend_thread_locked(rt_thread_t thread, rt_futex_t futex)
{
    rt_err_t rc;
    rc = rt_thread_suspend_with_flag(thread, RT_INTERRUPTIBLE);

    if (rc == RT_EOK)
    {
        /**
         * Brief: Add current thread into futex waiting thread list
         *
         * Note: Critical Section
         * - the futex waiting_thread list (RW)
         */
        rt_list_insert_before(&(futex->waiting_thread), &(thread->tlist));
    }

    return rc;
}

static int _futex_wait(rt_futex_t futex, struct rt_lwp *lwp, int *uaddr,
                       int value, const struct timespec *timeout, int op_flags)
{
    rt_tick_t to;
    rt_thread_t thread;
    rt_err_t rc = -RT_EINTR;

    /**
     * Brief: Remove current thread from scheduler, besides appends it to
     * the waiting thread list of the futex. If the timeout is specified
     * a timer will be setup for current thread
     *
     * Note: Critical Section
     * - futex.waiting (RW; Protected by lwp_lock)
     * - the local cpu
     */
    _futex_lock(lwp, op_flags);
    if (*uaddr == value)
    {
        thread = rt_thread_self();

        if (timeout)
        {
            to = timeout->tv_sec * RT_TICK_PER_SECOND;
            to += (timeout->tv_nsec * RT_TICK_PER_SECOND) / NANOSECOND_PER_SECOND;

            if (to < 0)
            {
                rc = -EINVAL;
                _futex_unlock(lwp, op_flags);
            }
            else
            {
                rt_enter_critical();
                rc = _suspend_thread_timeout_locked(thread, futex, to);
                _futex_unlock(lwp, op_flags);
                rt_exit_critical();
            }
        }
        else
        {
            rt_enter_critical();
            rc = _suspend_thread_locked(thread, futex);
            _futex_unlock(lwp, op_flags);
            rt_exit_critical();
        }

        if (rc == RT_EOK)
        {
            /* do schedule */
            rt_schedule();
            /* check errno */
            rc = rt_get_errno();
            rc = rc > 0 ? -rc : rc;
        }
    }
    else
    {
        _futex_unlock(lwp, op_flags);
        rc = -EAGAIN;
        rt_set_errno(EAGAIN);
    }

    return rc;
}

static long _futex_wake(rt_futex_t futex, struct rt_lwp *lwp, int number, int op_flags)
{
    long woken_cnt = 0;
    int is_empty = 0;
    rt_thread_t thread;

    /**
     * Brief: Wakeup a suspended thread on the futex waiting thread list
     *
     * Note: Critical Section
     * - the futex waiting_thread list (RW)
     */
    while (number && !is_empty)
    {
        _futex_lock(lwp, op_flags);
        is_empty = rt_list_isempty(&(futex->waiting_thread));
        if (!is_empty)
        {
            thread = rt_list_entry(futex->waiting_thread.next, struct rt_thread, tlist);
            /* remove from waiting list */
            rt_list_remove(&(thread->tlist));

            thread->error = RT_EOK;
            /* resume the suspended thread */
            rt_thread_resume(thread);

            number--;
            woken_cnt++;
        }
        _futex_unlock(lwp, op_flags);
    }

    /* do schedule */
    rt_schedule();
    return woken_cnt;
}


/**
 *  Brief: Wake up to nr_wake futex1 threads.
 *      If there are more waiters waiting on futex1 than nr_wake,
 *      insert the remaining at most nr_requeue waiters waiting
 *      on futex1 into the waiting queue of futex2.
*/
static long _futex_requeue(rt_futex_t futex1, rt_futex_t futex2, struct rt_lwp *lwp, 
                           int nr_wake, int nr_requeue, int opflags)
{
    long rtn;
    long woken_cnt = 0;
    int is_empty = 0;
    rt_thread_t thread;

    if (futex1 == futex2)
    {
        return -EINVAL;
    }

    /**
     * Brief: Wakeup a suspended thread on the futex waiting thread list
     *
     * Note: Critical Section
     * - the futex waiting_thread list (RW)
     */
    _futex_lock(lwp, opflags);
    while (nr_wake && !is_empty)
    {
        is_empty = rt_list_isempty(&(futex1->waiting_thread));
        if (!is_empty)
        {
            thread = rt_list_entry(futex1->waiting_thread.next, struct rt_thread, tlist);
            /* remove from waiting list */
            rt_list_remove(&(thread->tlist));

            thread->error = RT_EOK;
            /* resume the suspended thread */
            rt_thread_resume(thread);

            nr_wake--;
            woken_cnt++;
        }
    }
    rtn = woken_cnt;
    is_empty = 0;
    
    /**
     * Brief: Requeue
     *
     * Note: Critical Section
     * - the futex waiting_thread list (RW)
     */
    while (!is_empty && nr_requeue)
    {
        is_empty = rt_list_isempty(&(futex1->waiting_thread));
        if (!is_empty)
        {
            thread = rt_list_entry(futex1->waiting_thread.next, struct rt_thread, tlist);
            rt_list_remove(&(thread->tlist));
            rt_list_insert_before(&(futex2->waiting_thread), &(thread->tlist));
            nr_requeue--;
            rtn++;
        }
    }
    _futex_unlock(lwp, opflags);

    /* do schedule */
    rt_schedule();

    return rtn;
}

#include <syscall_generic.h>

rt_inline rt_bool_t _timeout_ignored(int op)
{
    /**
     * if (op & (FUTEX_WAKE|FUTEX_FD|FUTEX_WAKE_BITSET|FUTEX_TRYLOCK_PI|FUTEX_UNLOCK_PI)) was TRUE
     * `timeout` should be ignored by implementation, according to POSIX futex(2) manual.
     * since only FUTEX_WAKE is implemented in rt-smart, only FUTEX_WAKE was omitted currently
     */
    return ((op & (FUTEX_WAKE)) || (op & (FUTEX_REQUEUE)) || (op & (FUTEX_CMP_REQUEUE)));
}

sysret_t sys_futex(int *uaddr, int op, int val, const struct timespec *timeout,
                   int *uaddr2, int val3)
{
    struct rt_lwp *lwp = RT_NULL;
    sysret_t ret = 0;

    if (!lwp_user_accessable(uaddr, sizeof(int)))
    {
        ret = -EFAULT;
    }
    else if (timeout && !_timeout_ignored(op) && !lwp_user_accessable((void *)timeout, sizeof(struct timespec)))
    {
        ret = -EINVAL;
    }
    else
    {
        lwp = lwp_self();
        ret = lwp_futex(lwp, uaddr, op, val, timeout, uaddr2, val3);
    }

    return ret;
}

#define FUTEX_FLAGS (FUTEX_PRIVATE | FUTEX_CLOCK_REALTIME)
rt_err_t lwp_futex(struct rt_lwp *lwp, int *uaddr, int op, int val,
                   const struct timespec *timeout, int *uaddr2, int val3)
{
    rt_futex_t futex, futex2;
    rt_err_t rc = 0;
    int op_type = op & ~FUTEX_FLAGS;
    int op_flags = op & FUTEX_FLAGS;

    futex = _futex_get(uaddr, lwp, op_flags, &rc);
    if (!rc)
    {
        switch (op_type)
        {
            case FUTEX_WAIT:
                rc = _futex_wait(futex, lwp, uaddr, val, timeout, op_flags);
                break;
            case FUTEX_WAKE:
                rc = _futex_wake(futex, lwp, val, op_flags);
                break;
            case FUTEX_REQUEUE:
                futex2 = _futex_get(uaddr2, lwp, op_flags, &rc);
                if (!rc)
                { 
                    rc = _futex_requeue(futex, futex2, lwp, val, (long)timeout, op_flags);
                }
                break;
            case FUTEX_CMP_REQUEUE:
                _futex_lock(lwp, op_flags);
                if (*uaddr == val3)
                {
                    rc = 0;
                }
                else
                {
                    rc = -EAGAIN;
                }
                _futex_unlock(lwp, op_flags);
                if (rc == 0)
                {
                    futex2 = _futex_get(uaddr2, lwp, op_flags, &rc);
                    if (!rc)
                    { 
                        rc = _futex_requeue(futex, futex2, lwp, val, (long)timeout, op_flags);
                    }
                }
                break;
            default:
                LOG_W("User require op=%d which is not implemented", op);
                rc = -ENOSYS;
                break;
        }
    }

    return rc;
}
