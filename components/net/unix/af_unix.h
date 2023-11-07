#ifndef __AF_UNIX_H__
#define __AF_UNIX_H__

#include <rtthread.h>
#include <rtdef.h>
#include <dfs_file.h>
#include <sal_socket.h>
#include <sal_netdb.h>

#define LISTTEN_MAX_NUM 1024

#define TIMEOUT_MAX_TIME 100
#define UNIX_MSG_MAX_BYTE 102400

struct unix_sock;
struct msg_buf;
struct unix_conn;

struct msg_buf
{
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
    sa_family_t family;
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

int unix_socket(int domain, int type, int protocol);
int unix_socketpair(int domain, int type, int protocol, int *fds);
int unix_bind(int s, const struct sockaddr *name, socklen_t namelen);
int unix_listen(int s, int backlog);
int unix_connect(int s, const struct sockaddr *name, socklen_t namelen);
int unix_accept(int s, struct sockaddr *addr, socklen_t *addrlen);
int unix_sendto(int s, const void *data, size_t size, int flags, const struct sockaddr *to, socklen_t tolen);
int unix_sendmsg(int s, const struct msghdr *message, int flags);
int unix_recvmsg(int s, struct msghdr *message, int flags);
int unix_recvfrom(int s, void *mem, size_t len, int flags, struct sockaddr *from, socklen_t *fromlen);
int unix_getsockopt(int s, int level, int optname, void *optval, socklen_t *optlen);
int unix_setsockopt(int s, int level, int optname, const void *optval, socklen_t optlen);
int unix_shutdown(int s, int how);
int unix_getpeername(int s, struct sockaddr *name, socklen_t *namelen);
int unix_getsockname(int s, struct sockaddr *name, socklen_t *namelen);
int unix_poll(struct dfs_file *file, struct rt_pollreq *req);
int unix_close(int s);

#endif
