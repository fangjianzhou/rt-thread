#include <rtthread.h>

unsigned long rt_basic_processing_counter;
volatile unsigned long rt_basic_processing_array[1024];

void rt_basic_processing_thread_0_entry(void *param);
void rt_basic_processing_thread_report(void *param);
void rt_basic_processing_initialize(void);

void rt_basic_processing_initialize(void)
{
    rt_thread_t tid1 = rt_thread_create("test1", rt_basic_processing_thread_0_entry, RT_NULL, 10240, 10, 5);

    if (tid1 != RT_NULL)
        rt_thread_startup(tid1);
    
    rt_thread_resume(tid1);

    rt_thread_t tid2 = rt_thread_create("test2", rt_basic_processing_thread_report, RT_NULL, 10240, 2, 5);
    if (tid2 != RT_NULL)
        rt_thread_startup(tid2);

    rt_thread_resume(tid2);
}
MSH_CMD_EXPORT(rt_basic_processing_initialize, processing_initialize);


void rt_basic_processing_thread_0_entry(void *param)
{
    int i;

    for (i = 0; i < 1024; i++)
    {
        rt_basic_processing_array[i] = 0;
    }

    while (1)
    {
        for (i = 0; i < 1024; i++)
        {
            rt_basic_processing_array[i] =  (rt_basic_processing_array[i] + rt_basic_processing_counter) ^ rt_basic_processing_array[i];
        }

        rt_basic_processing_counter++;
    }
}

void  rt_basic_processing_thread_report(void *param)
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
        rt_kprintf("**** Thread-Metric Basic Single Thread Processing Test **** Relative Time: %lu\n", relative_time);

        /* See if there are any errors.  */
        if (rt_basic_processing_counter == last_counter)
        {

            rt_kprintf("ERROR: Invalid counter value(s). Basic processing thread died!\n");
        }

        /* Show the time period total.  */
        rt_kprintf("Time Period Total:  %lu\n\n", rt_basic_processing_counter - last_counter);

        /* Save the last counter.  */
        last_counter =  rt_basic_processing_counter;
    }
}
