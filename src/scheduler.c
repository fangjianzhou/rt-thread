/*
 * Copyright (c) 2023-2023, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2023-09-26     xqyjlj       the first version
 */

#include <rtthread.h>

#ifdef RT_USING_SMP
extern struct rt_scheduler rt_mp_scheduler_ops;
static struct rt_scheduler *_rt_sched_class = &rt_mp_scheduler_ops;
#else
extern struct rt_scheduler rt_up_scheduler_ops;
static struct rt_scheduler *_rt_sched_class = &rt_up_scheduler_ops;
#endif
void rt_system_scheduler_setup(struct rt_scheduler *sched)
{
    RT_ASSERT(sched != NULL);
#if defined(RT_USING_HOOK) && defined(RT_HOOK_USING_FUNC_PTR)
    RT_ASSERT(sched->sethook != NULL);
    RT_ASSERT(sched->switch_sethook != NULL);
#endif
    RT_ASSERT(sched->init != NULL);
    RT_ASSERT(sched->start != NULL);
    RT_ASSERT(sched->schedule != NULL);

#ifdef RT_USING_SMP
    RT_ASSERT(sched->schedule_do_irq != NULL);
#endif

    RT_ASSERT(sched->insert_thread != NULL);
    RT_ASSERT(sched->remove_thread != NULL);
    RT_ASSERT(sched->enter_critical != NULL);
    RT_ASSERT(sched->exit_critical != NULL);
    RT_ASSERT(sched->critical_level != NULL);

    _rt_sched_class = sched;
}

#if defined(RT_USING_HOOK) && defined(RT_HOOK_USING_FUNC_PTR)

/**
 * @addtogroup Hook
 */

/**@{*/

/**
 * @brief This function will set a hook function, which will be invoked when thread
 *        switch happens.
 *
 * @param hook is the hook function.
 */
void rt_scheduler_sethook(void (*hook)(struct rt_thread *from, struct rt_thread *to))
{
    _rt_sched_class->sethook(hook);
}

/**
 * @brief This function will set a hook function, which will be invoked when context
 *        switch happens.
 *
 * @param hook is the hook function.
 */
void rt_scheduler_switch_sethook(void (*hook)(struct rt_thread *tid))
{
    _rt_sched_class->switch_sethook(hook);
}

/**@}*/
#endif /* RT_USING_HOOK */

/**
 * @brief This function will initialize the system scheduler.
 */
void rt_system_scheduler_init(void)
{
    _rt_sched_class->init();
}

/**
 * @addtogroup Thread
 * @cond
 */

/**@{*/

/**
 * @brief This function will startup the scheduler. It will select one thread
 *        with the highest priority level, then switch to it.
 */
void rt_system_scheduler_start(void)
{
    _rt_sched_class->start();
}

/**
 * @brief This function will perform one scheduling. It will select one thread
 *        with the highest priority level in global ready queue or local ready queue,
 *        then switch to it.
 */
void rt_schedule(void)
{
    _rt_sched_class->schedule();
}

/**
 * @brief This function will handle IPI interrupt and do a scheduling in system.
 *
 * @param vector is the number of IPI interrupt for system scheduling.
 *
 * @param param is not used, and can be set to RT_NULL.
 *
 * @note this function should be invoke or register as ISR in BSP.
 */
void rt_scheduler_ipi_handler(int vector, void *param)
{
    rt_schedule();
}

#ifdef RT_USING_SMP
/**
 * @brief This function checks whether a scheduling is needed after an IRQ context switching. If yes,
 *        it will select one thread with the highest priority level, and then switch
 *        to it.
 */
void rt_scheduler_do_irq_switch(void *context)
{
    _rt_sched_class->schedule_do_irq(context);
}
#endif /* RT_USING_SMP */

/**
 * @brief This function will insert a thread to the system ready queue. The state of
 *        thread will be set as READY and the thread will be removed from suspend queue.
 *
 * @param thread is the thread to be inserted.
 *
 * @note  Please do not invoke this function in user application.
 */
void rt_schedule_insert_thread(struct rt_thread *thread)
{
    _rt_sched_class->insert_thread(thread);
}

/**
 * @brief This function will remove a thread from system ready queue.
 *
 * @param thread is the thread to be removed.
 *
 * @note  Please do not invoke this function in user application.
 */
void rt_schedule_remove_thread(struct rt_thread *thread)
{
    _rt_sched_class->remove_thread(thread);
}

/**
 * @brief This function will lock the thread scheduler.
 */
void rt_enter_critical(void)
{
    _rt_sched_class->enter_critical();
}
RTM_EXPORT(rt_enter_critical);

/**
 * @brief This function will unlock the thread scheduler.
 */
void rt_exit_critical(void)
{
    _rt_sched_class->exit_critical();
}
RTM_EXPORT(rt_exit_critical);

/**
 * @brief Get the scheduler lock level.
 *
 * @return the level of the scheduler lock. 0 means unlocked.
 */
rt_uint16_t rt_critical_level(void)
{
    return _rt_sched_class->critical_level();
}
RTM_EXPORT(rt_critical_level);

/**@}*/
/**@endcond*/
