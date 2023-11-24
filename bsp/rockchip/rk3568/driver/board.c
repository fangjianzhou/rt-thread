/*
 * Copyright (c) 2006-2023, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-3-08      GuEe-GUI     the first version
 */

#include <rthw.h>
#include <rtthread.h>

#include <mmu.h>
#include <psci.h>
#include <gicv3.h>
#include <gtimer.h>
#include <cpuport.h>
#include <interrupt.h>
#include <ioremap.h>
#include <board.h>
#include <drv_uart.h>
#include "mm_page.h"
#include <setup.h>


void rt_hw_board_init(void)
{
    rt_fdt_commit_memregion_early(&(rt_region_t)
    {
        .name = "memheap",
        .start = (rt_size_t)rt_kmem_v2p(HEAP_BEGIN),
        .end = (rt_size_t)rt_kmem_v2p(HEAP_END),
    }, RT_TRUE);

    rt_hw_common_setup();

}

void reboot(void)
{
    psci_system_reboot();

    void *cur_base = rt_ioremap((void *) CRU_BASE, 0x100);
    HWREG32(cur_base + 0x00D4) = 0xfdb9;
    HWREG32(cur_base + 0x00D8) = 0xeca8;
    
}
MSH_CMD_EXPORT(reboot, reboot...);

static void print_cpu_id(int argc, char *argv[])
{
    rt_kprintf("rt_hw_cpu_id:%d\n", rt_hw_cpu_id());
}
MSH_CMD_EXPORT_ALIAS(print_cpu_id, cpuid, print_cpu_id);

#ifdef RT_USING_AMP
void start_cpu(int argc, char *argv[])
{
    rt_uint32_t status;
    status = rt_psci_cpu_on(0x3, (rt_uint64_t) 0x7A000000);
    rt_kprintf("arm_psci_cpu_on 0x%X\n", status);
}
MSH_CMD_EXPORT(start_cpu, start_cpu);

#ifdef RT_AMP_SLAVE
void rt_hw_cpu_shutdown()
{
    rt_psci_cpu_off(0);
}
#endif /* RT_AMP_SLAVE */
#endif /* RT_USING_AMP */

#if defined(RT_USING_SMP) || defined(RT_USING_AMP)
rt_uint64_t rt_cpu_mpidr_early[] =
{
    [0] = 0x80000000,
    [1] = 0x80000100,
    [2] = 0x80000200,
    [3] = 0x80000300,
    [RT_CPUS_NR] = 0
};
#endif

#ifdef RT_USING_SMP
extern unsigned long MMUTable[];

void rt_hw_secondary_cpu_bsp_start(void)
{
    rt_hw_spin_lock(&_cpus_lock);
    rt_hw_mmu_ktbl_set((unsigned long)MMUTable);
    arm_gic_cpu_init(0, platform_get_gic_cpu_base());
    arm_gic_redist_init(0, platform_get_gic_redist_base());
    rt_hw_vector_init();
    rt_hw_gtimer_local_enable();
    arm_gic_umask(0, IRQ_ARM_IPI_KICK);

    rt_kprintf("\rcall cpu %d on success\n", rt_hw_cpu_id());

    rt_system_scheduler_start();
}

#endif
