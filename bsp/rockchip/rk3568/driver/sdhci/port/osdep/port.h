#ifndef _SDHCI_PORT_H
#define _SDHCI_PORT_H

#include <rtdef.h>
#include <rthw.h>

#include <string.h>
#include <drivers/platform.h>

#include "_sys/types.h"

#define CONFIG_MMC_SDHCI_IO_ACCESSORS

/*  */
#define __iomem
#define ____cacheline_aligned
#define unlikely(x) (x)
#define __exit
#define __init

#define container_of(ptr, type, member) \
    ((type *)((char *)(ptr) - (unsigned long)(&((type *)0)->member)))

#define __packed                        __attribute__((__packed__))
#define __aligned(x)                    __attribute__((__aligned__(x)))

#define __AC(X,Y)	(X##Y)
#define _AC(X,Y)	__AC(X,Y)

#define _UL(x)		(_AC(x, UL))
#define _ULL(x)		(_AC(x, ULL))

#define UL(x)		(_UL(x))
#define ULL(x)		(_ULL(x))

/* bits */
#define GENMASK_INPUT_CHECK(h, l)    0
#define BITS_PER_LONG (sizeof(long) * 8)

#define __GENMASK(h, l) \
	(((~UL(0)) - (UL(1) << (l)) + 1) & \
	 (~UL(0) >> (BITS_PER_LONG - 1 - (h))))
#define GENMASK(h, l) \
	(GENMASK_INPUT_CHECK(h, l) + __GENMASK(h, l))

#define le32_to_cpu(x) (x)
#define le16_to_cpu(x) (x)
#define cpu_to_le16(x) (x)
#define cpu_to_le32(x) (x)
#define lower_32_bits(n) ((u32)((n) & 0xffffffff))
#define upper_32_bits(n) ((u32)(((n) >> 16) >> 16))

#define __bf_shf(x) (__builtin_ffsll(x) - 1)
#define __BF_FIELD_CHECK(...)

#define FIELD_GET(_mask, _reg)						\
	({								\
        __BF_FIELD_CHECK(_mask, _reg, 0U, "FIELD_GET: ");	\
		(typeof(_mask))(((_reg) & (_mask)) >> __bf_shf(_mask));	\
	})

#define FIELD_PREP(_mask, _val)						\
	({								\
		__BF_FIELD_CHECK(_mask, 0ULL, _val, "FIELD_PREP: ");	\
		((typeof(_mask))(_val) << __bf_shf(_mask)) & (_mask);	\
	})

#define BIT(nr)			((1) << (nr))

/* errno */
#include <errno.h>

#ifndef EPROBE_DEFER
#define EPROBE_DEFER	517
#endif

/* device */
struct device
{
	const char *init_name;
    void *of_node;
	void *priv;
	void* dma_mask;
	void *userdata;
};

struct platform_device
{
	struct device dev;
    int irq;
	unsigned long base;
};

static inline void platform_set_drvdata(struct platform_device *pdev,
					void *data)
{
	pdev->dev.priv = data;
}

static inline void *platform_get_drvdata(const struct platform_device *pdev)
{
    return pdev->dev.priv;
}

/* sizes */
#define SZ_64K				0x00010000

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

/* printk */
#ifdef SDHCI_NO_PRINTK
#define printk(...)
#else
#define printk rt_kprintf
#endif

#ifdef SDHCI_PR_DEBUG
#define pr_debug printk
#else
#define pr_debug(...)
#endif
#define pr_err printk
#define pr_warn printk
#ifdef SDHCI_PR_INFO
#define pr_info printk
#else
#define pr_info(...)
#endif
#define pr_todo() ({rt_kprintf("TODO: %s:%d\n", __func__, __LINE__);})

#define dev_err(dev, fmt, ...) printk(fmt, ##__VA_ARGS__)
#define dev_dbg(...)

struct regulator {
    const char *supply_name;
};

/* export */
#define EXPORT_SYMBOL_GPL(...)
#define module_init(...)
#define module_exit(...)
#define module_param(...)
#define MODULE_AUTHOR(...)
#define MODULE_DESCRIPTION(...)
#define MODULE_LICENSE(...)
#define MODULE_PARM_DESC(...)

/* pm */
#define pm_runtime_put_noidle(...)
#define pm_runtime_get_noresume(...)

#define MMC_PM_KEEP_POWER	(1 << 0)	/* preserve card power during suspend */
#define MMC_PM_WAKE_SDIO_IRQ	(1 << 1)	/* wake up host system on SDIO IRQ assertion */

/* spinlock */
#ifdef RT_USING_SMP
#define spinlock_t rt_hw_spinlock_t
#else
#define spinlock_t long
#endif

void __sdhci_spin_lock_init(spinlock_t *l);
void __sdhci_spin_lock(spinlock_t *l);
void __sdhci_spin_unlock(spinlock_t *l);
void __sdhci_spin_irqsave(spinlock_t *l, unsigned long *f);
void __sdhci_spin_irqrestore(spinlock_t *l, unsigned long f);

#define spin_lock_init __sdhci_spin_lock_init
#define spin_lock __sdhci_spin_lock
#define spin_unlock __sdhci_spin_unlock
#define spin_lock_irqsave(l, f) __sdhci_spin_irqsave(l, &(f))
#define spin_unlock_irqrestore(l, f) __sdhci_spin_irqrestore(l, f)

#define __releases(...)
#define __acquires(...)

/* irq */
#include "_sys/irq.h"

/* config */
#define IS_ENABLED(x) 0
#define IS_REACHABLE(x) 0

/* timer/time */
#include "_sys/time.h"

#define from_timer(var, callback_timer, timer_fieldname) \
	container_of(callback_timer, typeof(*var), timer_fieldname)

/* io */
#ifndef writel
#define writel ___writel
static inline void ___writel(u32 value, volatile void *addr)
{
    *(volatile u32 *)addr = value;
}
#endif

#ifndef writew
#define writew ___writew
static inline void ___writew(u16 value, volatile void *addr)
{
    *(volatile u16 *)addr = value;
}
#endif

#ifndef writeb
#define writeb ___writeb
static inline void ___writeb(u8 value, volatile void *addr)
{
	*(volatile u8 *)addr = value;
}
#endif

#ifndef readb
#define readb ___readb
static inline u8 ___readb(const volatile void *addr)
{
	return *(const volatile u8 *)addr;
}
#endif

#ifndef readw
#define readw ___readw
static inline u16 ___readw(const volatile void *addr)
{
	return *(const volatile u16 *)addr;
}
#endif

#ifndef readl
#define readl ___readl
static inline u32 ___readl(const volatile void *addr)
{
	return *(const volatile u32 *)addr;
}
#endif

#define ioread32 readl
#define iowrite32 writel

extern void __iounmap(volatile void __iomem *addr);
#define iounmap __iounmap

extern unsigned long __virt_to_phys(volatile void *address);
#define virt_to_phys __virt_to_phys

extern void *__ioremap(phys_addr_t offset, size_t size);
#define ioremap __ioremap

#define IO_TLB_SEGSIZE	128
#define IO_TLB_SHIFT 11

unsigned int swiotlb_max_segment(void);

/* workqueue */
#include "_sys/work.h"

/* waitqueue */
#include "_sys/wait.h"

/* bug */
#define __WARN() printk("WARN: %s:%d\n", __FILE__, __LINE__)

#define WARN_ON(condition) ({						\
	int __ret_warn_on = !!(condition);				\
	if (unlikely(__ret_warn_on))					\
		__WARN();						\
	unlikely(__ret_warn_on);					\
})

#define BUG()	do {								\
	printk("BUG: failure at %s:%d/%s()!\n", __FILE__, __LINE__, __func__); \
	__builtin_trap();							\
} while (0)

#define BUG_ON(condition) do { if (unlikely(condition)) BUG(); } while (0)

#ifndef WARN
#define WARN(condition, format...) ({					\
	int __ret_warn_on = !!(condition);				\
	if (unlikely(__ret_warn_on))					\
		printk(format);			\
	unlikely(__ret_warn_on);					\
})
#endif

#define WARN_ONCE(condition, format...) WARN(condition, format)
#define WARN_ON_ONCE(condition) WARN_ON(condition)

# define fallthrough                    do {} while (0)

/* err */
#define MAX_ERRNO	4095

#define IS_ERR_VALUE(x) unlikely((unsigned long)(void *)(x) >= (unsigned long)-MAX_ERRNO)

static inline bool IS_ERR(const void *ptr)
{
	return IS_ERR_VALUE((unsigned long)ptr);
}

static inline void * ERR_PTR(long error)
{
	return (void *) error;
}

static inline long PTR_ERR(const void *ptr)
{
	return (long) ptr;
}

/* minmax */
#define min_t(type, x, y)	(((type)x < (type)y)? x : y)
#define max_t(type, x, y)	(((type)x > (type)y)? x : y)
#define min(x, y) ((x) < (y)? (x) : (y))

/* mem */
#define GFP_KERNEL 0
#define __GFP_ZERO    0x100u

/* regulator */
#define regulator_disable(...)
#define regulator_enable(...) ({0;})
int regulator_get_current_limit(struct regulator *regulator);
int regulator_is_supported_voltage(struct regulator *regulator,
				   int min_uV, int max_uV);

/* sg */
#include "_sys/sg.h"

/* dma */
#include "_sys/dma.h"

/* math */
#define DIV_ROUND_UP(n, d) (((n) + (d) - 1) / (d))

#define do_div(n,base) ({					\
	uint32_t __base = (base);				\
	uint32_t __rem;						\
	__rem = ((uint64_t)(n)) % __base;			\
	(n) = ((uint64_t)(n)) / __base;				\
	__rem;							\
 })

/* clk */
struct clk
{
	int dummy;
};

/* of */
#include "_sys/of.h"

#define might_sleep_if(cond)

#define read_poll_timeout(op, val, cond, sleep_us, timeout_us, \
				sleep_before_read, args...) \
({ \
	u64 __timeout_us = (timeout_us); \
	unsigned long __sleep_us = (sleep_us); \
	ktime_t __timeout = ktime_add_us(ktime_get(), __timeout_us); \
	might_sleep_if((__sleep_us) != 0); \
	if (sleep_before_read && __sleep_us) \
		usleep_range((__sleep_us >> 2) + 1, __sleep_us); \
	for (;;) { \
		(val) = op(args); \
		if (cond) \
			break; \
		if (__timeout_us && \
		    ktime_compare(ktime_get(), __timeout) > 0) { \
			(val) = op(args); \
			break; \
		} \
		if (__sleep_us) \
			usleep_range((__sleep_us >> 2) + 1, __sleep_us); \
	} \
	(cond) ? 0 : -ETIMEDOUT; \
})

#define readx_poll_timeout(op, addr, val, cond, sleep_us, timeout_us)	\
	read_poll_timeout(op, val, cond, sleep_us, timeout_us, false, addr)

#define readl_poll_timeout(addr, val, cond, delay_us, timeout_us) \
	readx_poll_timeout(readl, addr, val, cond, delay_us, timeout_us)

const char *dev_name(const struct rt_device *dev);
void* devm_platform_ioremap_resource(struct platform_device *pdev, unsigned int index);
int platform_get_irq(struct platform_device *dev, unsigned int index);
void *devm_kzalloc(struct rt_device *dev, size_t size, gfp_t gfp);

void *devm_kmalloc(struct rt_device *dev, size_t size, gfp_t gfp);

extern void *kmap_atomic(struct page *page);
void kunmap_atomic(struct page *page);

extern int __sdhci_ilog2(int v);
#define ilog2 __sdhci_ilog2

#endif /* _SDHCI_PORT_H */
