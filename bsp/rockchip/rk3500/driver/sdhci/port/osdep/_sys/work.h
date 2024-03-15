#ifndef _SDHCI_WORK_H
#define _SDHCI_WORK_H

#include <ipc/workqueue.h>

struct work_struct;

typedef void (*work_func_t)(struct work_struct *work);

#define workqueue_struct rt_workqueue

struct work_struct
{
    struct rt_work _wk;
    work_func_t func;

    void *priv;
};

void _init_work(struct work_struct *w, work_func_t f);
#define INIT_WORK(_work, _func) _init_work(_work, _func)
struct workqueue_struct *alloc_workqueue(const char *fmt,
                     unsigned int flags,
                     int max_active, ...);

bool queue_work(struct workqueue_struct *wq,
                  struct work_struct *work);
void destroy_workqueue(struct workqueue_struct *wq);

#endif
