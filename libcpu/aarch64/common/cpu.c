/*
 * Copyright (c) 2006-2019, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2011-09-15     Bernard      first version
 * 2019-07-28     zdzn         add smp support
 */

#include <rthw.h>
#include <rtthread.h>
#include <board.h>
#include "cp15.h"

#define DBG_TAG "libcpu.aarch64.cpu"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>
#include <string.h>
#include "cpu.h"
#include "psci_api.h"

void (*system_off)(void);

#ifdef RT_USING_SMP

#define REPORT_ERR(retval) LOG_E("got error code %d in %s(), %s:%d", (retval), __func__, __FILE__, __LINE__)
#define CHECK_RETVAL(retval) if (retval) {REPORT_ERR(retval);}
#define cpuid_to_hwid(cpuid) \
    ((((cpuid) >= 0) && ((cpuid) < RT_CPUS_NR)) ? rt_cpu_mpidr_early[cpuid] : ID_ERROR)
#define set_hwid(cpuid, hwid) \
    ((((cpuid) >= 0) && ((cpuid) < RT_CPUS_NR)) ? (rt_cpu_mpidr_early[cpuid] = (hwid)) : ID_ERROR)
#define get_cpu_node(cpuid) \
    ((((cpuid) >= 0) && ((cpuid) < RT_CPUS_NR)) ? _cpu_node[cpuid] : NULL)
#define set_cpu_node(cpuid, node) \
    ((((cpuid) >= 0) && ((cpuid) < RT_CPUS_NR)) ? (_cpu_node[cpuid] = node) : NULL)

#ifdef RT_USING_FDT
#include "dtb_node.h"
struct dtb_node *_cpu_node[RT_CPUS_NR];
#endif /* RT_USING_FDT */

#define MPIDR_AFF_MASK 0x000000FF00FFFFFFul
#define REPORT_ERR(retval) LOG_E("got error code %d in %s(), %s:%d", (retval), __func__, __FILE__, __LINE__)
#define CHECK_RETVAL(retval) if (retval) {REPORT_ERR(retval);}

/**
 * cpu_ops_tbl contains cpu_ops_t for each cpu kernel observed,
 * given cpu logical id 'i', its cpu_ops_t is 'cpu_ops_tbl[i]'
 */
struct cpu_ops_t *cpu_ops_tbl[RT_CPUS_NR];

#ifdef RT_USING_SMART
// _id_to_mpidr is a table translate logical id to mpid, which is a 64-bit value
rt_uint64_t rt_cpu_mpidr_early[RT_CPUS_NR] rt_weak = {[0 ... RT_CPUS_NR - 1] = ID_ERROR};
#else
/* The more common mpidr_el1 table, redefine it in BSP if it is in other cases */
rt_weak rt_uint64_t rt_cpu_mpidr_early[] =
{
    [0] = 0x80000000,
    [1] = 0x80000001,
    [2] = 0x80000002,
    [3] = 0x80000003,
    [4] = 0x80000004,
    [5] = 0x80000005,
    [6] = 0x80000006,
    [7] = 0x80000007,
    [RT_CPUS_NR] = 0
};
#endif /* RT_USING_SMART */

typedef rt_hw_spinlock_t arch_spinlock_t;

static inline void arch_spin_lock(arch_spinlock_t *lock)
{
    unsigned int tmp;

    asm volatile(
    "   sevl\n"
    "1: wfe\n"
    "2: ldaxr   %w0, %1\n"
    "   cbnz    %w0, 1b\n"
    "   stxr    %w0, %w2, %1\n"
    "   cbnz    %w0, 2b\n"
    : "=&r" (tmp), "+Q" (lock->lock)
    : "r" (1)
    : "cc", "memory");
}

static inline int arch_spin_trylock(arch_spinlock_t *lock)
{
    unsigned int tmp;

    asm volatile(
    "  ldaxr   %w0, %1\n"
    "  cbnz    %w0, 1f\n"
    "  stxr    %w0, %w2, %1\n"
    "1:\n"
    : "=&r" (tmp), "+Q" (lock->lock)
    : "r" (1)
    : "cc", "memory");

    return !tmp;
}

static inline void arch_spin_unlock(arch_spinlock_t *lock)
{
    asm volatile(
    " stlr    %w1, %0\n"
    : "=Q" (lock->lock) : "r" (0) : "memory");
}

void rt_hw_spin_lock_init(arch_spinlock_t *lock)
{
    lock->lock = 0;
}

void rt_hw_spin_lock(rt_hw_spinlock_t *lock)
{
    arch_spin_lock(lock);
}

void rt_hw_spin_unlock(rt_hw_spinlock_t *lock)
{
    arch_spin_unlock(lock);
}

rt_bool_t rt_hw_spin_trylock(rt_hw_spinlock_t *lock)
{
    return arch_spin_trylock(lock);
}

static int _cpus_init_data_hardcoded(int num_cpus, rt_uint64_t *cpu_hw_ids, struct cpu_ops_t *cpu_ops[])
{
    // load in cpu_hw_ids in cpuid_to_hwid,
    // cpu_ops to cpu_ops_tbl
    if (num_cpus > RT_CPUS_NR)
    {
        LOG_W("num_cpus (%d) greater than RT_CPUS_NR (%d)\n", num_cpus, RT_CPUS_NR);
        num_cpus = RT_CPUS_NR;
    }

    for (int i = 0; i < num_cpus; i++)
    {
        set_hwid(i, cpu_hw_ids[i]);
        cpu_ops_tbl[i] = cpu_ops[i];
    }
    return 0;
}

#ifdef RT_USING_FDT

/** read ('size' * 4) bytes number from start, big-endian format */
static rt_uint64_t _read_be_number(void *start, int size)
{
    rt_uint64_t buf = 0;
    for (; size > 0; size--)
    {
        buf = (buf << 32) | fdt32_to_cpu(*(uint32_t *)start);
        start = (uint32_t *)start + 1;
    }
    return buf;
}

/** check device-type of the node, */
static bool _node_is_cpu(struct dtb_node *node)
{
    char *device_type = dtb_node_get_dtb_node_property_value(node, "device_type", NULL);
    if (device_type)
    {
        return !strcmp(device_type, "cpu");
    }
    return false;
}

static int _read_and_set_hwid(struct dtb_node *cpu, int *id_pool, int *pcpuid)
{
    // size/address_cells is number of elements in reg array
    int size;
    static int address_cells, size_cells;
    if (!address_cells && !size_cells)
        dtb_node_get_dtb_node_cells(cpu, &address_cells, &size_cells);

    void *id_start = dtb_node_get_dtb_node_property_value(cpu, "reg", &size);
    rt_uint64_t mpid = _read_be_number(id_start, address_cells);

    *pcpuid = *id_pool;
    *id_pool = *id_pool + 1;
    set_hwid(*pcpuid, mpid);

    LOG_I("Using MPID 0x%lx as cpu %d", mpid, *pcpuid);

    // setting _cpu_node for cpu_init use
    _cpu_node[*pcpuid] = cpu;

    return 0;
}

static int _read_and_set_cpuops(struct dtb_node *cpu, int cpuid)
{
    char *method = dtb_node_get_dtb_node_property_value(cpu, "enable-method", NULL);
    if (!method)
    {
        LOG_E("Cannot read method from cpu node");
        return -1;
    }

    struct cpu_ops_t *cpu_ops;
    if (!strcmp(method, cpu_ops_psci.method))
    {
        cpu_ops = &cpu_ops_psci;
    }
    else if (!strcmp(method, cpu_ops_spin_tbl.method))
    {
        cpu_ops = &cpu_ops_spin_tbl;
    }
    else
    {
        cpu_ops = RT_NULL;
        LOG_E("Not supported cpu_ops: %s", method);
    }
    cpu_ops_tbl[cpuid] = cpu_ops;

    LOG_D("Using boot method [%s] for cpu %d", cpu_ops->method, cpuid);
    return 0;
}

static int _cpus_init_data_fdt()
{
    // cpuid_to_hwid and cpu_ops_tbl with fdt
    void *root = get_dtb_node_head();
    int id_pool = 0;
    int cpuid;
    struct dtb_node *cpus = dtb_node_get_dtb_node_by_path(root, "/cpus");

    // for each cpu node (device-type is cpu), read its mpid and set its cpuid_to_hwid
    for_each_node_child(cpus)
    {
        if (!_node_is_cpu(cpus))
        {
            continue;
        }

        if (id_pool > RT_CPUS_NR)
        {
            LOG_W("Reading more cpus from FDT than RT_CPUS_NR"
                "\n  Parsing will not continue and only %d cpus will be used.", RT_CPUS_NR);
            break;
        }

        _read_and_set_hwid(cpus, &id_pool, &cpuid);

        _read_and_set_cpuops(cpus, cpuid);
    }
    return 0;
}

#endif /* RT_USING_FDT */

/** init cpu with hardcoded infomation or parsing from FDT */
static int _cpus_init(int num_cpus, rt_uint64_t *cpu_hw_ids, struct cpu_ops_t *cpu_ops[])
{
    int retval;

    // first setup cpu_ops_tbl and cpuid_to_hwid
    if (num_cpus > 0)
        retval = _cpus_init_data_hardcoded(num_cpus, cpu_hw_ids, cpu_ops);
    else
    {
        retval = -1;
#ifdef RT_USING_FDT
        retval = _cpus_init_data_fdt();
#endif
    }

    if (retval)
        return retval;

    // using cpuid_to_hwid and cpu_ops_tbl to call method_init and cpu_init
    // assuming that cpuid 0 has already init
    for (int i = 1; i < RT_CPUS_NR; i++)
    {
        if (cpuid_to_hwid(i) == ID_ERROR)
        {
            LOG_E("Failed to find hardware id of CPU %d", i);
            continue;
        }

        if (cpu_ops_tbl[i] && cpu_ops_tbl[i]->cpu_init)
        {
            retval = cpu_ops_tbl[i]->cpu_init(i, RT_NULL);
            CHECK_RETVAL(retval);
        }
        else
        {
            LOG_E("Failed to find cpu_init for cpu %d with cpu_ops[%p], cpu_ops->cpu_init[%p]"
                , cpuid_to_hwid(i), cpu_ops_tbl[i], cpu_ops_tbl[i] ? cpu_ops_tbl[i]->cpu_init : NULL);
        }
    }
    return 0;
}

/**
 * @brief boot cpu with hardcoded data
 *
 * @param num_cpus number of cpus
 * @param cpu_hw_ids each element represents a hwid of cpu[i]
 * @param cpu_ops each element represents a pointer to cpu_ops of cpu[i]
 * @return int 0 on success,
 */
int rt_hw_cpu_boot_secondary(int num_cpus, rt_uint64_t *cpu_hw_ids, struct cpu_ops_t *cpu_ops[])
{
    int retval = 0;
    if (num_cpus < 1 || !cpu_hw_ids || !cpu_ops)
        return -1;

    retval = _cpus_init(num_cpus, cpu_hw_ids, cpu_ops);
    CHECK_RETVAL(retval);

    return retval;
}

#define CPU_INIT_USING_FDT 0,0,0

/**
 * @brief Initialize cpu infomation from fdt
 *
 * @return int
 */
int rt_hw_cpu_init()
{
#ifdef RT_USING_FDT
    return _cpus_init(CPU_INIT_USING_FDT);
#else
    LOG_E("CPU init failed since RT_USING_FDT was not defined");
    return -0xa; /* no fdt support */
#endif /* RT_USING_FDT */
}

rt_weak void rt_hw_secondary_cpu_idle_exec(void)
{
    asm volatile("wfe" ::
                     : "memory", "cc");
}

#endif /*RT_USING_SMP*/

/**
 * @addtogroup ARM CPU
 */
/*@{*/

const char *rt_hw_cpu_arch(void)
{
    return "aarch64";
}

/** shutdown CPU */
void rt_hw_cpu_shutdown(void)
{
    rt_uint32_t level;
    rt_kprintf("shutdown...\n");

    if (system_off)
        system_off();
    LOG_E("system shutdown failed");

    level = rt_hw_interrupt_disable();
    while (level)
    {
        RT_ASSERT(0);
    }
}
MSH_CMD_EXPORT_ALIAS(rt_hw_cpu_shutdown, shutdown, shutdown machine);

#ifdef RT_USING_CPU_FFS
/**
 * This function finds the first bit set (beginning with the least significant bit)
 * in value and return the index of that bit.
 *
 * Bits are numbered starting at 1 (the least significant bit).  A return value of
 * zero from any of these functions means that the argument was zero.
 *
 * @return return the index of the first bit set. If value is 0, then this function
 * shall return 0.
 */
int __rt_ffs(int value)
{
    return __builtin_ffs(value);
}
#endif
/*@}*/
