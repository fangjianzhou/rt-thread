#include <rtthread.h>
#include <mmu.h>
#include <interrupt.h>

static rt_ubase_t kernel_ram_start = 0x40000000;

void jump_to_zephyr(void)
{
    rt_kprintf("disable IRQ\n");
    rt_hw_local_irq_disable();

    rt_kprintf("\rJump to kernel...\n");

    void (*kernel_entry)();
    kernel_entry = (void(*)())kernel_ram_start;

    kernel_entry();

    __builtin_unreachable();
}
MSH_CMD_EXPORT(jump_to_zephyr, load zephyr);