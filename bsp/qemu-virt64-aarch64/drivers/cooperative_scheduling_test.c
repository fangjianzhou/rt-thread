#include <rtthread.h>
/* Define the counters used in the demo application...  */

unsigned long   tm_cooperative_thread_0_counter;
unsigned long   tm_cooperative_thread_1_counter;
unsigned long   tm_cooperative_thread_2_counter;
unsigned long   tm_cooperative_thread_3_counter;
unsigned long   tm_cooperative_thread_4_counter;


/* Define the test thread prototypes.  */

void            tm_cooperative_thread_0_entry(void *param);
void            tm_cooperative_thread_1_entry(void *param);
void            tm_cooperative_thread_2_entry(void *param);
void            tm_cooperative_thread_3_entry(void *param);
void            tm_cooperative_thread_4_entry(void *param);


/* Define the reporting thread prototype.  */

void            tm_cooperative_thread_report(void *param);


/* Define the initialization prototype.  */

void            tm_cooperative_scheduling_initialize(void);


/* Define the cooperative scheduling test initialization.  */

void  tm_cooperative_scheduling_initialize(void)
{
    /* Create all 5 threads at priority 3.  */
    rt_thread_t tid0 = rt_thread_create("test0", tm_cooperative_thread_0_entry, RT_NULL, 10240, 22, 5);
    if (tid0 != RT_NULL)
        rt_thread_startup(tid0);

    //tm_thread_create(1, 3, tm_cooperative_thread_1_entry);
    rt_thread_t tid1 = rt_thread_create("test1", tm_cooperative_thread_1_entry, RT_NULL, 10240, 22, 5);
    if (tid1 != RT_NULL)
        rt_thread_startup(tid1);
    
    rt_thread_t tid2 = rt_thread_create("test2", tm_cooperative_thread_2_entry, RT_NULL, 10240, 22, 5);
    if (tid2 != RT_NULL)
        rt_thread_startup(tid2);

    rt_thread_t tid3 = rt_thread_create("test3", tm_cooperative_thread_3_entry, RT_NULL, 10240, 22, 5);
    if (tid3 != RT_NULL)
        rt_thread_startup(tid3);
    rt_thread_t tid4 = rt_thread_create("test4", tm_cooperative_thread_4_entry, RT_NULL, 10240, 22, 5);
    if (tid4 != RT_NULL)
        rt_thread_startup(tid4);
    /* Resume all 5 threads.  */
    rt_thread_resume(tid0);
    rt_thread_resume(tid1);
    rt_thread_resume(tid2);
    rt_thread_resume(tid3);
    rt_thread_resume(tid4);
    /* Create the reporting thread. It will preempt the other 
       threads and print out the test results.  */
    rt_thread_t tid5 = rt_thread_create("test5", tm_cooperative_thread_report, RT_NULL, 10240, 21, 5);
    if (tid5 != RT_NULL)
    {
        rt_thread_startup(tid5);
    }

    rt_thread_resume(tid5);
}
MSH_CMD_EXPORT(tm_cooperative_scheduling_initialize, cooperative scheduling test);

/* Define the first cooperative thread.  */
void  tm_cooperative_thread_0_entry(void *param)
{

    while(1)
    {
      
        /* Relinquish to all other threads at same priority.  */
        //tm_thread_relinquish();
        rt_thread_yield();
        /* Increment this thread's counter.  */
        tm_cooperative_thread_0_counter++;
    }
}

/* Define the second cooperative thread.  */
void  tm_cooperative_thread_1_entry(void *param)
{

    while(1)
    {

        /* Relinquish to all other threads at same priority.  */
        rt_thread_yield();

        /* Increment this thread's counter.  */
        tm_cooperative_thread_1_counter++;
    }
}

/* Define the third cooperative thread.  */
void  tm_cooperative_thread_2_entry(void *param)
{

    while(1)
    {

        /* Relinquish to all other threads at same priority.  */
        rt_thread_yield();

        /* Increment this thread's counter.  */
        tm_cooperative_thread_2_counter++;
    }
}


/* Define the fourth cooperative thread.  */
void  tm_cooperative_thread_3_entry(void *param)
{

    while(1)
    {

        /* Relinquish to all other threads at same priority.  */
        rt_thread_yield();
        /* Increment this thread's counter.  */
        tm_cooperative_thread_3_counter++;
    }
}


/* Define the fifth cooperative thread.  */
void  tm_cooperative_thread_4_entry(void *param)
{

    while(1)
    {

        /* Relinquish to all other threads at same priority.  */
        rt_thread_yield();

        /* Increment this thread's counter.  */
        tm_cooperative_thread_4_counter++;
    }
}


/* Define the cooperative test reporting thread.  */
void  tm_cooperative_thread_report(void *param)
{

    rt_kprintf("tm_cooperative_thread_report\n");
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
        rt_kprintf("**** Thread-Metric Cooperative Scheduling Test **** Relative Time: %lu\n", relative_time);

        /* Calculate the total of all the counters.  */
        total =  tm_cooperative_thread_0_counter + tm_cooperative_thread_1_counter + tm_cooperative_thread_2_counter
                    + tm_cooperative_thread_3_counter + tm_cooperative_thread_4_counter;

        /* Calculate the average of all the counters.  */
        average =  total/5;
		
		/* WCC - integrity check */
		rt_kprintf("tm_cooperative_thread_0_counter: %d\n", tm_cooperative_thread_0_counter);
		rt_kprintf("tm_cooperative_thread_1_counter: %d\n", tm_cooperative_thread_1_counter);
		rt_kprintf("tm_cooperative_thread_2_counter: %d\n", tm_cooperative_thread_2_counter);
		rt_kprintf("tm_cooperative_thread_3_counter: %d\n", tm_cooperative_thread_3_counter);
		rt_kprintf("tm_cooperative_thread_4_counter: %d\n", tm_cooperative_thread_4_counter);

        /* See if there are any errors.  */
        if ((tm_cooperative_thread_0_counter < (average - 1)) || 
            (tm_cooperative_thread_0_counter > (average + 1)) ||
            (tm_cooperative_thread_1_counter < (average - 1)) || 
            (tm_cooperative_thread_1_counter > (average + 1)) ||
            (tm_cooperative_thread_2_counter < (average - 1)) || 
            (tm_cooperative_thread_2_counter > (average + 1)) ||
            (tm_cooperative_thread_3_counter < (average - 1)) || 
            (tm_cooperative_thread_3_counter > (average + 1)) ||
            (tm_cooperative_thread_4_counter < (average - 1)) || 
            (tm_cooperative_thread_4_counter > (average + 1)))
        {

            rt_kprintf("ERROR: Invalid counter value(s). Cooperative counters should not be more that 1 different than the average!\n");
        }

        /* Show the time period total.  */
        rt_kprintf("Time Period Total:  %lu\n\n", total - last_total);

        /* Save the last total.  */
        last_total =  total;
    }
}
