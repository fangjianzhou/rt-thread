#include <rtthread.h>


/* Define the counters used in the demo application...  */

unsigned long   tm_preemptive_thread_0_counter;
unsigned long   tm_preemptive_thread_1_counter;
unsigned long   tm_preemptive_thread_2_counter;
unsigned long   tm_preemptive_thread_3_counter;
unsigned long   tm_preemptive_thread_4_counter;


/* Define the test thread prototypes.  */

void            tm_preemptive_thread_0_entry(void *param);
void            tm_preemptive_thread_1_entry(void *param);
void            tm_preemptive_thread_2_entry(void *param);
void            tm_preemptive_thread_3_entry(void *param);
void            tm_preemptive_thread_4_entry(void *param);


/* Define the reporting thread prototype.  */

void            tm_preemptive_thread_report(void *param);


/* Define the initialization prototype.  */

void            tm_preemptive_scheduling_initialize(void);



/* Define the preemptive scheduling test initialization.  */
rt_thread_t tid1 = RT_NULL;
rt_thread_t tid2 = RT_NULL;
rt_thread_t tid3 = RT_NULL;
rt_thread_t tid4 = RT_NULL;
void  tm_preemptive_scheduling_initialize(void)
{

    /* Create thread 0 at priority 10.  */
    rt_thread_t tid0 = rt_thread_create("test0", tm_preemptive_thread_0_entry, RT_NULL, 10240, 25, 5);
    if ( tid0 != RT_NULL)
        rt_thread_startup(tid0);
    /* Create thread 1 at priority 9.  */
    rt_thread_t tid1 = rt_thread_create("test1", tm_preemptive_thread_1_entry, RT_NULL, 10240, 24 ,5);
    if ( tid1 != RT_NULL)
        rt_thread_startup(tid1);
    /* Create thread 2 at priority 8.  */
    rt_thread_t tid2 = rt_thread_create("test2", tm_preemptive_thread_2_entry, RT_NULL, 10240, 23, 5);

    if ( tid2 != RT_NULL)
        rt_thread_startup(tid2);
    /* Create thread 3 at priority 7.  */
    rt_thread_t tid3 = rt_thread_create("test3", tm_preemptive_thread_3_entry, RT_NULL, 10240, 22, 5);
    if ( tid3 != RT_NULL)
        rt_thread_startup(tid3);
    /* Create thread 4 at priority 6.  */
    rt_thread_t tid4 = rt_thread_create("test4", tm_preemptive_thread_4_entry, RT_NULL, 10240, 21, 5);
    if ( tid4 != RT_NULL)
        rt_thread_startup(tid4);
    /* Resume just thread 0.  */
    rt_thread_resume(tid0);

    /* Create the reporting thread. It will preempt the other 
       threads and print out the test results.  */
    rt_thread_t tid5 = rt_thread_create("test5", tm_preemptive_thread_report, RT_NULL, 10240, 2, 5);
    if ( tid5 != RT_NULL)
        rt_thread_startup(tid5);
    rt_thread_resume(tid5);
}
MSH_CMD_EXPORT(tm_preemptive_scheduling_initialize, interrupt processing);

/* Define the first preemptive thread.  */
void  tm_preemptive_thread_0_entry(void *param)
{

    while(1)
    {

        /* Resume thread 1.  */
        rt_thread_resume(tid1);

        /* We won't get back here until threads 1, 2, 3, and 4 all execute and
           self-suspend.  */

        /* Increment this thread's counter.  */
        tm_preemptive_thread_0_counter++;
    }
}

/* Define the second preemptive thread.  */
void  tm_preemptive_thread_1_entry(void *param)
{

    while(1)
    {

        /* Resume thread 2.  */
        rt_thread_resume(tid2);

        /* We won't get back here until threads 2, 3, and 4 all execute and
           self-suspend.  */

        /* Increment this thread's counter.  */
        tm_preemptive_thread_1_counter++;

        /* Suspend self!  */
        rt_thread_suspend(tid1);
    }
}

/* Define the third preemptive thread.  */
void  tm_preemptive_thread_2_entry(void *param)
{

    while(1)
    {

        /* Resume thread 3.  */
        rt_thread_resume(tid3);

        /* We won't get back here until threads 3 and 4 execute and
           self-suspend.  */

        /* Increment this thread's counter.  */
        tm_preemptive_thread_2_counter++;

        /* Suspend self!  */
        rt_thread_suspend(tid2);
    }
}


/* Define the fourth preemptive thread.  */
void  tm_preemptive_thread_3_entry(void *param)
{

    while(1)
    {

        /* Resume thread 4.  */
        rt_thread_resume(tid4);

        /* We won't get back here until thread 4 executes and
           self-suspends.  */

        /* Increment this thread's counter.  */
        tm_preemptive_thread_3_counter++;

        /* Suspend self!  */
        rt_thread_suspend(tid3);
    }
}


/* Define the fifth preemptive thread.  */
void  tm_preemptive_thread_4_entry(void *param)
{

    while(1)
    {

        /* Increment this thread's counter.  */
        tm_preemptive_thread_4_counter++;

        /* Self suspend thread 4.  */
        rt_thread_suspend(tid4);
    }
}

#define TM_TEST_DURATION 30
/* Define the preemptive test reporting thread.  */
void  tm_preemptive_thread_report(void *param)
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
        rt_thread_delay(TM_TEST_DURATION);

        /* Increment the relative time.  */
        relative_time =  relative_time + TM_TEST_DURATION;

        /* Print results to the stdio window.  */
        rt_kprintf("**** Thread-Metric Preemptive Scheduling Test **** Relative Time: %lu\n", relative_time);

        /* Calculate the total of all the counters.  */
        total =  tm_preemptive_thread_0_counter + tm_preemptive_thread_1_counter + tm_preemptive_thread_2_counter
                    + tm_preemptive_thread_3_counter + tm_preemptive_thread_4_counter;

        /* Calculate the average of all the counters.  */
        average =  total/5;

        /* See if there are any errors.  */
        if ((tm_preemptive_thread_0_counter < (average - 1)) || 
            (tm_preemptive_thread_0_counter > (average + 1)) ||
            (tm_preemptive_thread_1_counter < (average - 1)) || 
            (tm_preemptive_thread_1_counter > (average + 1)) ||
            (tm_preemptive_thread_2_counter < (average - 1)) || 
            (tm_preemptive_thread_2_counter > (average + 1)) ||
            (tm_preemptive_thread_3_counter < (average - 1)) || 
            (tm_preemptive_thread_3_counter > (average + 1)) ||
            (tm_preemptive_thread_4_counter < (average - 1)) || 
            (tm_preemptive_thread_4_counter > (average + 1)))
        {

            rt_kprintf("ERROR: Invalid counter value(s). Preemptive counters should not be more that 1 different than the average!\n");
        }

        /* Show the time period total.  */
        rt_kprintf("Time Period Total:  %lu\n\n", total - last_total);

        /* Save the last total.  */
        last_total =  total;
    }
}
