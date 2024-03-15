#include <osdep/port.h>

unsigned int jiffies_to_msecs(const unsigned long j)
{
    return j * (1000u / RT_TICK_PER_SECOND);
}

unsigned long msecs_to_jiffies(const unsigned int m)
{
    return rt_tick_from_millisecond((rt_int32_t)m);
}

unsigned long nsecs_to_jiffies(rt_uint64_t n)
{
    pr_todo();
    return 0;
}

unsigned __sdhci_hz(void)
{
    return RT_TICK_PER_SECOND;
}

unsigned long __sdhci_jiffies(void)
{
    return rt_tick_get();
}

static void _timer_cb(void *p)
{
    struct timer_list *timer = (struct timer_list *)p;

    if (timer->function)
        timer->function(timer);
}

void __init_timer(struct timer_list *timer,
            void (*func)(struct timer_list *), unsigned int flags,
            const char *name)
{
    timer->function = func;
    rt_timer_init(&timer->tmr, name, _timer_cb, timer, 0, RT_TIMER_FLAG_SOFT_TIMER);
}

int mod_timer(struct timer_list *timer, unsigned long expires)
{
    rt_tick_t tick = rt_tick_get();

    if (expires < tick)
    {
        WARN_ON(1);
        expires = tick;
    }
    tick = expires - tick;

    rt_timer_stop(&timer->tmr);
    rt_timer_control(&timer->tmr, RT_TIMER_CTRL_SET_TIME, &tick);

    return rt_timer_start(&timer->tmr);
}

int del_timer(struct timer_list * timer)
{
    return rt_timer_stop(&timer->tmr);
}

int ktime_compare(const ktime_t cmp1, const ktime_t cmp2)
{
    if (cmp1 < cmp2)
        return -1;
    if (cmp1 > cmp2)
        return 1;
    return 0;
}

bool ktime_after(const ktime_t cmp1, const ktime_t cmp2)
{
    return ktime_compare(cmp1, cmp2) > 0;
}

ktime_t ktime_add_us(const ktime_t kt, const rt_uint64_t usec)
{
    return ktime_add_ns(kt, usec * NSEC_PER_USEC);
}

ktime_t ktime_get(void)
{
    return jiffies_to_msecs(jiffies) * NSEC_PER_MSEC;
}

ktime_t ktime_add_ms(const ktime_t kt, const rt_uint64_t msec)
{
    return ktime_add_ns(kt, msec * NSEC_PER_MSEC);
}

void __sdhci_udelay(unsigned long u)
{
    u *= 100;
    while (u>0) u--;
}

void usleep_range(unsigned long min, unsigned long max)
{
    rt_int32_t msecs;

    msecs = (min + max)/2000;
    if (msecs)
        rt_thread_mdelay(msecs);
    else
        udelay((min + max)/2);
}
