#include <rtthread.h>
#include <interrupt.h>
/* Define the counters used in the demo application...  */

unsigned long   tm_interrupt_preemption_thread_0_counter;
unsigned long   tm_interrupt_preemption_thread_1_counter;
unsigned long   tm_interrupt_preemption_handler_counter;


/* Define the test thread prototypes.  */

void            tm_interrupt_preemption_thread_0_entry(void *param);
void            tm_interrupt_preemption_thread_1_entry(void *param);
void            tm_interrupt_preemption_handler_entry(void);


/* Define the reporting thread prototype.  */

void            tm_interrupt_preemption_thread_report(void *param);


/* Define the interrupt handler.  This must be called from the RTOS.  */

void            tm_interrupt_preemption_handler(int vector, void *param);


/* Define the initialization prototype.  */

void            tm_interrupt_preemption_processing_initialize(void);




/* Define the interrupt processing test initialization.  */
rt_thread_t tid0 = RT_NULL;
void  tm_interrupt_preemption_processing_initialize(void)
{
    rt_hw_interrupt_umask(10);
    rt_hw_ipi_handler_install(10,  tm_interrupt_preemption_handler);
    /* Create interrupt thread at priority 3.  */
    tid0 = rt_thread_create("test0", tm_interrupt_preemption_thread_0_entry, RT_NULL, 10240, 3, 5);
    if (tid0 != RT_NULL)
        rt_thread_startup(tid0);
    
    /* Create thread that generates the interrupt at priority 10.  */
    rt_thread_t tid1 = rt_thread_create("test1", tm_interrupt_preemption_thread_1_entry, RT_NULL, 10240, 10, 5);
    if (tid1 != RT_NULL)
        rt_thread_startup(tid1);

    /* Resume just thread 1.  */
    rt_thread_resume(tid1);

    /* Create the reporting thread. It will preempt the other 
       threads and print out the test results.  */
    rt_thread_t tid5 = rt_thread_create("test5", tm_interrupt_preemption_thread_report, RT_NULL, 10240, 2, 5);
    if (tid5 != RT_NULL)
        rt_thread_startup(tid5);

    rt_thread_resume(tid5);
}
MSH_CMD_EXPORT(tm_interrupt_preemption_processing_initialize, cooperative scheduling test);

/* Define the interrupt thread.  This thread is resumed from the 
   interrupt handler.  It runs and suspends.  */
void  tm_interrupt_preemption_thread_0_entry(void *param)
{

    while(1)
    {

        /* Increment this thread's counter.  */
        tm_interrupt_preemption_thread_0_counter++;

        /* Suspend. This will allow the thread generating the 
           interrupt to run again.  */
        rt_thread_suspend(tid0);
    }
}

/* Define the thread that generates the interrupt.  */
void  tm_interrupt_preemption_thread_1_entry(void *param)
{

    while(1)
    {

        /* Force an interrupt. The underlying RTOS must see that the 
           the interrupt handler is called from the appropriate software
           interrupt or trap. */
        rt_hw_ipi_send(10, 1<<rt_hw_cpu_id());

        tm_interrupt_preemption_thread_1_counter++;
    }
}


/* Define the interrupt handler.  This must be called from the RTOS trap handler.
   To be fair, it must behave just like a processor interrupt, i.e. it must save
   the full context of the interrupted thread during the preemption processing. */
void  tm_interrupt_preemption_handler(int vector, void *param)
{
    /* Increment the interrupt count.  */
    tm_interrupt_preemption_handler_counter++;

    /* Resume the higher priority thread from the ISR.  */
    rt_thread_resume(tid0);
}


/* Define the interrupt test reporting thread.  */
void  tm_interrupt_preemption_thread_report(void *param)
{

unsigned long   total;
unsigned long   relative_time;
unsigned long   last_total;
unsigned long   average;


    /* Initialize the last total.  */
    last_total =  0;

    /* Initialize the relative time.  */
    relative_time =  0;

    while(1)
    {

        /* Sleep to allow the test to run.  */
        rt_thread_delay(30);

        /* Increment the relative time.  */
        relative_time =  relative_time + 30;

        /* Print results to the stdio window.  */
        rt_kprintf("**** Thread-Metric Interrupt Preemption Processing Test **** Relative Time: %lu\n", relative_time);

        /* Calculate the total of all the counters.  */
        total =  tm_interrupt_preemption_thread_0_counter + tm_interrupt_preemption_thread_1_counter + tm_interrupt_preemption_handler_counter;

        /* Calculate the average of all the counters.  */
        average =  total/3;

        /* See if there are any errors.  */
        if ((tm_interrupt_preemption_thread_0_counter < (average - 1)) || 
            (tm_interrupt_preemption_thread_0_counter > (average + 1)) ||
            (tm_interrupt_preemption_thread_1_counter < (average - 1)) || 
            (tm_interrupt_preemption_thread_1_counter > (average + 1)) ||
            (tm_interrupt_preemption_handler_counter < (average - 1)) || 
            (tm_interrupt_preemption_handler_counter > (average + 1)))
        {

            rt_kprintf("ERROR: Invalid counter value(s). Interrupt processing test has failed!\n");
        }

        /* Show the total interrupts for the time period.  */
        rt_kprintf("Time Period Total:  %lu\n\n", tm_interrupt_preemption_handler_counter - last_total);

        /* Save the last total number of interrupts.  */
        last_total =  tm_interrupt_preemption_handler_counter;
    }
}
