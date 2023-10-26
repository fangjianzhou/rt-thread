/*
 * Copyright (c) 2023-2023, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2023-09-26     xqyjlj       the first version
 */

#ifndef __RT_SCHED_H__
#define __RT_SCHED_H__

#include <rtdef.h>

#ifdef __cplusplus
extern "C" {
#endif

struct rt_scheduler
{
#if defined(RT_USING_HOOK) && defined(RT_HOOK_USING_FUNC_PTR)
    void (*sethook)(void (*)(struct rt_thread *, struct rt_thread *));
    void (*switch_sethook)(void (*)(struct rt_thread *));
#endif /* RT_USING_HOOK */
    void        (*init)();
    void        (*start)();
    void        (*schedule)();
#ifdef RT_USING_SMP
    void        (*schedule_do_irq)(void *);
#endif
    void        (*insert_thread)(struct rt_thread *);
    void        (*remove_thread)(struct rt_thread *);
    void        (*enter_critical)();
    void        (*exit_critical)();
    rt_uint16_t (*critical_level)();
};

void rt_system_scheduler_setup(struct rt_scheduler *sched);
void rt_system_scheduler_init(void);
void rt_system_scheduler_start(void);

void rt_schedule(void);
void rt_schedule_insert_thread(struct rt_thread *thread);
void rt_schedule_remove_thread(struct rt_thread *thread);

void        rt_enter_critical(void);
void        rt_exit_critical(void);
rt_uint16_t rt_critical_level(void);

#ifdef RT_USING_HOOK
void rt_scheduler_sethook(void (*hook)(rt_thread_t from, rt_thread_t to));
void rt_scheduler_switch_sethook(void (*hook)(struct rt_thread *tid));
#endif /* RT_USING_HOOK */

#ifdef RT_USING_SMP
void rt_secondary_cpu_entry(void);
void rt_scheduler_do_irq_switch(void *context);
void rt_scheduler_ipi_handler(int vector, void *param);
#endif /* RT_USING_SMP */

#ifdef __cplusplus
}
#endif

#endif
