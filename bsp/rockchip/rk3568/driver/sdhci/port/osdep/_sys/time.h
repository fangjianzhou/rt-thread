#ifndef _SDHCI_TIME_H
#define _SDHCI_TIME_H

#include "types.h"

typedef int64_t ktime_t;

#define NSEC_PER_USEC	1000L
#define NSEC_PER_SEC	1000000000L
#define NSEC_PER_MSEC	1000000L

struct timer_list
{
	struct rt_timer tmr;
	void			(*function)(struct timer_list *);
	u32			flags;
};

void __init_timer(struct timer_list *timer,
            void (*func)(struct timer_list *), unsigned int flags,
            const char *name);
#define timer_setup(timer, callback, flags) __init_timer(timer, callback, flags, "tmr")
#define del_timer_sync(t) del_timer(t)
int del_timer(struct timer_list * timer);
int mod_timer(struct timer_list *timer, unsigned long expires);

unsigned long msecs_to_jiffies(const unsigned int m);

extern unsigned __sdhci_hz(void);
#define HZ __sdhci_hz()

extern unsigned long __sdhci_jiffies(void);
#define jiffies __sdhci_jiffies()
unsigned long nsecs_to_jiffies(u64 n);

#define ktime_add_ns(kt, nsval)		((kt) + (nsval))
bool ktime_after(const ktime_t cmp1, const ktime_t cmp2);
ktime_t ktime_get(void);
ktime_t ktime_add_ms(const ktime_t kt, const u64 msec);
int ktime_compare(const ktime_t cmp1, const ktime_t cmp2);
ktime_t ktime_add_us(const ktime_t kt, const u64 usec);

void mdelay(unsigned long x);
void msleep(unsigned int msecs);
void usleep_range(unsigned long min, unsigned long max);

extern void __sdhci_udelay(unsigned long u);

#define udelay __sdhci_udelay

#endif
