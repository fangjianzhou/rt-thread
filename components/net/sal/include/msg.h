
#ifndef __MSG_H__
#define __MSG_H__
#include <rtthread.h>

struct unix_sock;
struct msg_buf;
struct unix_conn;

struct msg_buf
{
    void *parm;
    void *buf;
    rt_size_t length;
    void *control_data;
    rt_size_t data_len;
    int msg_type;
    int data_type;
    int msg_level;
    int *fd;
    rt_slist_t msg_next;
    rt_slist_t msg_node;
};

struct last_buf
{
    void *buf;
    rt_size_t length;
    rt_size_t offset;
    struct msg_buf *msg;
};

struct unix_sock
{
    rt_uint8_t len;
    int flags;
    uint8_t family;
    char path[108];
    struct unix_conn *conn;
    rt_wqueue_t wq_head;
    rt_atomic_t listen_num;
    rt_atomic_t conn_counter;
    struct rt_mutex sock_lock;
    rt_slist_t wait_conn_head;
    struct last_buf pbuf;
};

struct unix_conn
{
    int state;
    int type;
    int proto;

    rt_uint32_t send_timeout;
    rt_uint32_t recv_timeout;
    rt_wqueue_t wq_read_head;
    rt_wqueue_t wq_confirm;
    struct rt_mutex conn_lock;
    rt_slist_t msg_head;
    rt_slist_t conn_node;
    struct unix_sock *sock;
    struct unix_sock *ser_sock;
    struct unix_conn *correl_conn;
    int (* conn_callback)(struct unix_conn *conn);
};

#endif
