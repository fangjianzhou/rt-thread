#include <rtthread.h>


/* Define the counters used in the demo application...  */

unsigned long   tm_message_processing_counter;
unsigned long   tm_message_sent[4];
unsigned long   tm_message_received[4];


/* Define the test thread prototypes.  */

void            tm_message_processing_thread_0_entry(void *param);


/* Define the reporting thread prototype.  */

void            tm_message_processing_thread_report(void *param);


/* Define the initialization prototype.  */

void            tm_message_processing_initialize(void);


/* Define the message processing test initialization.  */

static struct rt_messagequeue mq;
static rt_uint8_t msg_pool[2048];

void  tm_message_processing_initialize(void)
{
    rt_err_t result;

    result = rt_mq_init(&mq, "mqt", &msg_pool[0], 1, sizeof(msg_pool), RT_IPC_FLAG_PRIO);

    if ( result != RT_EOK )
    {
        rt_kprintf(" init message queue failed.\n");
        return;
    }

    /* Create thread 0 at priority 10.  */
    rt_thread_t tid0 = rt_thread_create("test0", tm_message_processing_thread_0_entry, RT_NULL, 10240, 10, 5);
    if (tid0 != RT_NULL)
        rt_thread_startup(tid0);

    /* Resume thread 0.  */
    rt_thread_resume(tid0);

    /* Create the reporting thread. It will preempt the other 
       threads and print out the test results.  */
    rt_thread_t tid5 = rt_thread_create("test5", tm_message_processing_thread_report, RT_NULL, 10240, 2, 5);
    if (tid5 != RT_NULL)
        rt_thread_startup(tid5);
 
    rt_thread_resume(tid5);
}
MSH_CMD_EXPORT(tm_message_processing_initialize, interrupt processing);

/* Define the message processing thread.  */
void  tm_message_processing_thread_0_entry(void *param)
{
    rt_err_t result; 
    /* Initialize the source message.   */
    tm_message_sent[0] =  0x11112222;
    tm_message_sent[1] =  0x33334444;
    tm_message_sent[2] =  0x55556666;
    tm_message_sent[3] =  0x77778888;

    while(1)
    {   	
        /* Send a message to the queue.  */
        result = rt_mq_send(&mq, tm_message_sent, sizeof(tm_message_sent));
        if ( result != RT_EOK)
        {
            rt_kprintf("rt_mq_send ERR\n");
            break;
        }
        /* Receive a message from the queue.  */
        result = rt_mq_recv(&mq, &tm_message_received, sizeof(tm_message_received), RT_WAITING_FOREVER);
        if ( result != RT_EOK)
        {
            rt_kprintf("rt_mq_recv ERR\n");
            break;
        }
        /* Check for invalid message.  */
        if (tm_message_received[3] != tm_message_sent[3])
            break;

        /* Increment the last word of the 16-byte message.  */
        tm_message_sent[3]++;

        /* Increment the number of messages sent and received.  */
        tm_message_processing_counter++;
    }
}


/* Define the message test reporting thread.  */
void  tm_message_processing_thread_report(void *param)
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
        rt_kprintf("**** Thread-Metric Message Processing Test **** Relative Time: %lu\n", relative_time);

        /* See if there are any errors.  */
        if (tm_message_processing_counter == last_counter)
        {

            rt_kprintf("ERROR: Invalid counter value(s). Error sending/receiving messages!\n");
        }

        /* Show the time period total.  */
        rt_kprintf("Time Period Total:  %lu\n\n", tm_message_processing_counter - last_counter);

        /* Save the last counter.  */
        last_counter =  tm_message_processing_counter;
    }
}
