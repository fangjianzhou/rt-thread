#include <osdep/port.h>

void __sdhci_spin_lock_init(spinlock_t *l)
{
#ifdef RT_USING_SMP
    rt_hw_spin_lock_init(l);
#endif
}

void __sdhci_spin_lock(spinlock_t *l)
{
#ifdef RT_USING_SMP
    rt_hw_spin_lock(l);
#endif
}

void __sdhci_spin_unlock(spinlock_t *l)
{
#ifdef RT_USING_SMP
    rt_hw_spin_unlock(l);
#endif
}

void __sdhci_spin_irqsave(spinlock_t *l, unsigned long *f)
{
    *f = rt_hw_interrupt_disable();
}

void __sdhci_spin_irqrestore(spinlock_t *l, unsigned long f)
{
    rt_hw_interrupt_enable(f);
}
