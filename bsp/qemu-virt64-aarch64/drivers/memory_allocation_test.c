 
#include <rtthread.h>


/* Define the counters used in the demo application...  */

unsigned long   tm_memory_allocation_counter;


/* Define the test thread prototypes.  */

void            tm_memory_allocation_thread_0_entry(void *param);


/* Define the reporting thread prototype.  */

void            tm_memory_allocation_thread_report(void *param);


/* Define the initialization prototype.  */

void            tm_memory_allocation_initialize(void);


static rt_uint8_t *ptr = RT_NULL;
static struct rt_mempool mp;
static rt_uint8_t mempool[4096];
/* Define the memory allocation processing test initialization.  */

void  tm_memory_allocation_initialize(void)
{
    
    rt_mp_init(&mp, "mp1", &mempool[0], sizeof(mempool), 80);
    /* Create thread 0 at priority 10.  */
    rt_thread_t tid0 = rt_thread_create("test0", tm_memory_allocation_thread_0_entry, RT_NULL, 10240, 10, 5);
    if (tid0 != RT_NULL)
        rt_thread_startup(tid0);

    /* Resume thread 0.  */
    rt_thread_resume(tid0);

    /* Create the reporting thread. It will preempt the other 
       threads and print out the test results.  */
    rt_thread_t tid5 = rt_thread_create("test5", tm_memory_allocation_thread_report, RT_NULL, 10240, 2, 5);
    if (tid5 != RT_NULL)
        rt_thread_startup(tid5);
    rt_thread_resume(tid5);
}
MSH_CMD_EXPORT(tm_memory_allocation_initialize, interrupt processing);

/* Define the memory allocation processing thread.  */
void  tm_memory_allocation_thread_0_entry(void *param)
{
    while(1)
    {

        /* Allocate memory from pool.  */
       ptr = rt_mp_alloc(&mp ,RT_WAITING_FOREVER);
       if (ptr == RT_NULL)
       {
            rt_kprintf("alloc failed\n");
            return;
       }

        /* Release the memory back to the pool.  */
        rt_mp_free(ptr);
        ptr = RT_NULL;

        /* Increment the number of memory allocations sent and received.  */
        tm_memory_allocation_counter++;
    }
}


/* Define the memory allocation test reporting thread.  */
void  tm_memory_allocation_thread_report(void *param)
{

unsigned long   last_counter;
unsigned long   relative_time;


    /* Initialize the last counter.  */
    last_counter =  0;

    /* Initialize the relative time.  */
    relative_time =  0;

    while(1)
    {

        /* Sleep to allow the test to run.  */
        rt_thread_delay(30);

        /* Increment the relative time.  */
        relative_time =  relative_time + 30;

        /* Print results to the stdio window.  */
        rt_kprintf("**** Thread-Metric Memory Allocation Test **** Relative Time: %lu\n", relative_time);

        /* See if there are any errors.  */
        if (tm_memory_allocation_counter == last_counter)
        {

            rt_kprintf("ERROR: Invalid counter value(s). Error allocating/deallocating memory!\n");
        }

        /* Show the time period total.  */
        rt_kprintf("Time Period Total:  %lu\n\n", tm_memory_allocation_counter - last_counter);

        /* Save the last counter.  */
        last_counter =  tm_memory_allocation_counter;
    }
}

