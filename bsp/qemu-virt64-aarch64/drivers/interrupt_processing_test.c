#include <rtthread.h>
#include <interrupt.h>

/* Define the counters used in the demo application...  */

unsigned long   tm_interrupt_thread_0_counter;
unsigned long   tm_interrupt_handler_counter;


/* Define the test thread prototypes.  */

void            tm_interrupt_thread_0_entry(void *param);


/* Define the reporting thread prototype.  */

void            tm_interrupt_thread_report(void *param);


/* Define the interrupt handler.  This must be called from the RTOS.  */

void            tm_interrupt_handler(int vector, void *param);


/* Define the initialization prototype.  */

void            tm_interrupt_processing_initialize(void);




/* Define the interrupt processing test initialization.  */
static rt_sem_t sem = RT_NULL;
void  tm_interrupt_processing_initialize(void)
{
    rt_hw_interrupt_umask(10);
    rt_hw_ipi_handler_install(10,  tm_interrupt_handler);

    /* Create thread that generates the interrupt at priority 10.  */
    rt_thread_t tid0 = rt_thread_create("test0", tm_interrupt_thread_0_entry, RT_NULL, 10240, 22, 5);
    if (tid0 != RT_NULL)
        rt_thread_startup(tid0);
    
    /* Create a semaphore that will be posted from the interrupt 
       handler.  */
    sem = rt_sem_create("dsem", 0, RT_IPC_FLAG_FIFO);
    if (sem == RT_NULL)
    {
        rt_kprintf("create semaphore failed.\n");
        return;
    }

    /* Resume just thread 0.  */
    rt_thread_resume(tid0);

    /* Create the reporting thread. It will preempt the other 
       threads and print out the test results.  */
    rt_thread_t tid5 = rt_thread_create("test5", tm_interrupt_thread_report, RT_NULL, 10240, 21, 5);
    if (tid5 != RT_NULL)
        rt_thread_startup(tid5);

    rt_thread_resume(tid5);
}
MSH_CMD_EXPORT(tm_interrupt_processing_initialize, interrupt processing);

/* Define the thread that generates the interrupt.  */
void  tm_interrupt_thread_0_entry(void *param)
{

    rt_err_t status;


    /* Pickup the semaphore since it is initialized to 1 by default. */
    status = rt_sem_take(sem, RT_WAITING_NO);

    /* Check for good status.  */
    if (status != RT_EOK)
        return;

    while(1)
    {

        /* Force an interrupt. The underlying RTOS must see that the 
           the interrupt handler is called from the appropriate software
           interrupt or trap. */
        rt_hw_ipi_send(10, 1<<rt_hw_cpu_id());

        /* We won't get back here until the interrupt processing is complete,
           including the setting of the semaphore from the interrupt 
           handler.  */

        /* Pickup the semaphore set by the interrupt handler. */
        status = rt_sem_take(sem, RT_WAITING_NO);

        /* Check for good status.  */
        if (status != RT_EOK)
            return;

        /* Increment this thread's counter.  */
        tm_interrupt_thread_0_counter++;
    }
}


/* Define the interrupt handler.  This must be called from the RTOS trap handler.
   To be fair, it must behave just like a processor interrupt, i.e. it must save
   the full context of the interrupted thread during the preemption processing. */
void  tm_interrupt_handler(int vector, void *param)
{

    /* Increment the interrupt count.  */
    tm_interrupt_handler_counter++;

    /* Put the semaphore from the interrupt handler.  */
    rt_sem_release(sem);
}


/* Define the interrupt test reporting thread.  */
void  tm_interrupt_thread_report(void *param)
{

unsigned long   total;
unsigned long   last_total;
unsigned long   relative_time;
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
        rt_kprintf("**** Thread-Metric Interrupt Processing Test **** Relative Time: %lu\n", relative_time);

        /* Calculate the total of all the counters.  */
        total =  tm_interrupt_thread_0_counter + tm_interrupt_handler_counter;

        /* Calculate the average of all the counters.  */
        average =  total/2;

        /* See if there are any errors.  */
        if ((tm_interrupt_thread_0_counter < (average - 1)) || 
            (tm_interrupt_thread_0_counter > (average + 1)) ||
            (tm_interrupt_handler_counter < (average - 1)) || 
            (tm_interrupt_handler_counter > (average + 1)))
        {

            rt_kprintf("ERROR: Invalid counter value(s). Interrupt processing test has failed!\n");
        }

        /* Show the total interrupts for the time period.  */
        rt_kprintf("Time Period Total:  %lu\n\n", tm_interrupt_handler_counter - last_total);

        /* Save the last total number of interrupts.  */
        last_total =  tm_interrupt_handler_counter;
    }
}
