/*
 * Copyright (c) 2006-2023, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2023-10-27     zmq      First version
 */

#include <poll.h>
#include <fcntl.h>
#include <sys/errno.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <af_unix.h>
#include <dfs_mnt.h>
#include <dfs_dentry.h>
#include <unistd.h>
#include <rtdef.h>
#include <netdev.h>

#define UNIXSOCK_BITS (SOCK_STREAM | SOCK_DGRAM)

/* The local virtual network device */
extern struct netdev *netdev_lo;

#define DBG_TAG "af_unix"
#define AF_UNIX_MUTEX_NAME "AF_UNIX_CONN"

#define CONN_NONE           0x0
#define CONN_LISTEN         0x1
#define CONN_CONNECT        0x10
#define CONN_CLOSE          0x100
#define CONN_SHUT_RD        0x1000
#define CONN_SHUT_WR        0x10000
#define CONN_SHUT_RDWR      0x100000

#define MSEC_TO_USEC 1000

static int unix_df_close(struct dfs_file *df);

static const struct dfs_file_ops unix_fops =
{
    .close      = unix_df_close,
};

static struct msg_buf *create_msgbuf()
{
    struct msg_buf *msg = RT_NULL;

    msg = rt_calloc(1, sizeof(struct msg_buf));
    if (!msg)
    {
        goto out;
    }

    msg->msg_type = 0;
    msg->buf = RT_NULL;
    msg->data_len = 0;
    msg->data_type = 0;
    msg->fd = RT_NULL;
    msg->control_data = RT_NULL;
    msg->msg_level = 0;
    rt_slist_init(&(msg->msg_next));
    rt_slist_init(&(msg->msg_node));

out:
    return msg;
}

static int conn_callback(struct unix_conn *conn)
{
    struct unix_conn *correl_conn = RT_NULL;

    rt_mutex_take(&(conn->conn_lock), RT_WAITING_FOREVER);
    correl_conn = conn->correl_conn;
    conn->state |= CONN_CONNECT;
    rt_wqueue_wakeup(&(conn->sock->wq_head), (void *)POLLIN);
    rt_mutex_release(&(conn->conn_lock));

    rt_mutex_take(&(correl_conn->conn_lock), RT_WAITING_FOREVER);
    correl_conn->state |= CONN_CONNECT;
    rt_mutex_release(&(correl_conn->conn_lock));

    return 0;
}

int unix_socketpair(int domain, int type, int protocol, int *fds)
{
    rt_err_t ret = -ENOMEM;
    struct dfs_file *dfa = RT_NULL;
    struct dfs_file *dfb = RT_NULL;
    struct unix_conn *conna = RT_NULL;
    struct unix_conn *connb = RT_NULL;
    struct unix_sock *socka = RT_NULL;
    struct unix_sock *sockb = RT_NULL;

    dfa = fd_get(fds[0]);
    if (!dfa)
        goto out;

    dfb = fd_get(fds[1]);
    if (!dfb)
        goto out;

    socka = dfa->vnode->data;
    RT_ASSERT(socka != RT_NULL)

    sockb = dfb->vnode->data;
    RT_ASSERT(sockb != RT_NULL)

    conna = socka->conn;
    RT_ASSERT(conna != RT_NULL)

    connb = sockb->conn;
    RT_ASSERT(connb != RT_NULL)

    conna->correl_conn = connb;
    connb->correl_conn = conna;

    conna->state |= CONN_CONNECT;
    connb->state |= CONN_CONNECT;

    ret = 0;

out:
    if (ret < 0)
    {
        rt_set_errno(-ret);
        ret = -1;
    }

    return ret;
}

static struct unix_conn *create_conn(int domain, int type, int protocol)
{
    struct unix_conn *conn = RT_NULL;
    struct unix_sock *sock = RT_NULL;

    conn = rt_calloc(1, sizeof(struct unix_conn));
    if (!conn)
        goto out;

    sock = rt_calloc(1, sizeof(struct unix_sock));
    if (!sock)
        goto sock_out;

    conn->type = type;
    conn->proto = protocol;
    conn->state = CONN_NONE;
    conn->send_timeout = 0;
    conn->recv_timeout = 0;
    conn->correl_conn = RT_NULL;
    conn->sock = RT_NULL;
    conn->conn_callback = conn_callback;
    conn->ser_sock = RT_NULL;
    conn->sock = sock;
    sock->conn = conn;

    rt_mutex_init(&(conn->conn_lock), AF_UNIX_MUTEX_NAME, RT_IPC_FLAG_FIFO);
    rt_wqueue_init(&(conn->wq_read_head));
    rt_wqueue_init(&(conn->wq_confirm));
    rt_slist_init(&(conn->msg_head));

    goto out;

sock_out:
    rt_free(conn);
    conn = RT_NULL;

out:

    return conn;
}

static void _close_fd_news(int *fd, int num)
{
    if (fd)
    {
        for (int i = 0; i < num; i++)
        {
            int nfd = dfs_dup_from(fd[i], RT_NULL);
            close(nfd);
        }
    }
}

static void _msg_release(struct msg_buf *msg)
{
    if (msg)
    {
        if (msg->buf)
        {
            rt_free(msg->buf);
            msg->buf = RT_NULL;
        }

        if (msg->fd)
        {
            _close_fd_news(msg->fd, ((msg->data_len - CMSG_LEN(0))/sizeof(int)));
            rt_free(msg->fd);
        }

        msg->fd = RT_NULL;
        msg->data_len = 0;

        if (msg->control_data)
        {
            rt_free(msg->control_data);
            msg->control_data = RT_NULL;
        }

        rt_free(msg);
    }
}

static void _msg_next_release(rt_slist_t *head)
{
    struct msg_buf *msg = RT_NULL;
    rt_slist_t *node = RT_NULL;
    if (head)
    {
        rt_slist_for_each(node,head)
        {
            rt_slist_remove(head, node);
            msg = rt_slist_entry(node, struct msg_buf, msg_next);
            _msg_release(msg);
        }
    }
}

static void _msg_node_release(rt_slist_t *head)
{
    struct msg_buf *msg = RT_NULL;
    rt_slist_t *node = RT_NULL;

    if (head)
    {
        rt_slist_for_each(node,head)
        {
            rt_slist_remove(head, node);
            msg = rt_slist_entry(node, struct msg_buf, msg_node);
            _msg_next_release(&(msg->msg_next));
            _msg_release(msg);
        }
    }
}

static void msg_buf_release(struct msg_buf *msg)
{
    if (msg)
    {
        _msg_node_release(&(msg->msg_node));
        _msg_next_release(&(msg->msg_next));
        _msg_release(msg);
    }
}

static void conn_release(struct unix_conn *conn)
{
    if (conn)
    {
        rt_mutex_take(&(conn->conn_lock), RT_WAITING_FOREVER);
        _msg_node_release(&(conn->msg_head));
        rt_mutex_release(&(conn->conn_lock));
        rt_mutex_detach(&(conn->conn_lock));
        rt_free(conn);
    }
}

static void sock_release(struct unix_sock *sock)
{
    rt_mutex_take(&(sock->sock_lock), RT_WAITING_FOREVER);
    if (sock->conn)
    {
        conn_release(sock->conn);
        sock->conn = RT_NULL;
    }

    if (sock->pbuf.buf)
    {
        rt_free(sock->pbuf.buf);
        sock->pbuf.buf = RT_NULL;
    }

    if (sock->pbuf.msg)
    {
        sock->pbuf.msg = RT_NULL;
        msg_buf_release(sock->pbuf.msg);
    }

    rt_mutex_release(&(sock->sock_lock));
    rt_mutex_detach(&(sock->sock_lock));

    rt_free(sock);
}

static int unix_validate_addr(struct sockaddr_un *name, socklen_t namelen)
{
    if (namelen <= offsetof(struct sockaddr_un, sun_path) || namelen > sizeof(struct sockaddr_un))
    {
        return -1;
    }

    return 0;
}

static void unix_sock_init(struct unix_sock *sock, int domain)
{
    sock->flags = 0;
    sock->family = domain;
    sock->pbuf.buf = RT_NULL;
    sock->pbuf.msg = RT_NULL;
    sock->pbuf.length = 0;
    sock->pbuf.offset = 0;

    rt_atomic_store(&(sock->listen_num), 0);
    rt_atomic_store(&(sock->conn_counter), 0);
    rt_slist_init(&(sock->wait_conn_head));
    rt_mutex_init(&(sock->sock_lock), AF_UNIX_MUTEX_NAME, RT_IPC_FLAG_FIFO);
    rt_wqueue_init(&(sock->wq_head));
}

int unix_socket(int domain, int type, int protocol)
{
    int fd;
    rt_err_t ret = -EINVAL;
    struct dfs_file *df;
    struct unix_conn *conn = RT_NULL;
    struct unix_sock *sock = RT_NULL;

    switch((type & UNIXSOCK_BITS))
    {
        case SOCK_STREAM :
        case SOCK_DGRAM :
            conn = create_conn(domain, type, protocol);
            break;
        default:
            goto out;
    }

    ret = -ENOBUFS;
    if (!conn)
        goto out;

    ret = -ENOMEM;
    fd = fd_new();
    if (fd < 0)
        goto fd_out;

    df = fd_get(fd);
    if (!df)
        goto df_out;

    df->vnode = (struct dfs_vnode *)rt_calloc(1, sizeof(struct dfs_vnode));
    if (!df->vnode)
        goto df_out;

    dfs_vnode_init(df->vnode, FT_SOCKET, &unix_fops);
#ifdef RT_USING_DFS_V2
    df->fops = &unix_fops;
#endif

    sock = conn->sock;
    df->vnode->data = sock;
    unix_sock_init(sock, domain);

    sock->flags |= (type & O_NONBLOCK);

    return fd;

df_out:
    fd_release(fd);

fd_out:
    sock = conn->sock;
    if (sock)
        rt_free(sock);

    conn_release(conn);

out:
    if (ret < 0)
    {
        rt_set_errno(-ret);
        ret = -1;
    }

    return ret;
}

static int unix_do_bind(struct dfs_file *df, char *path, rt_bool_t flag)
{
    int fd = 0;
    char *tmp = RT_NULL;
    rt_err_t ret = -ENOENT;
    char *fullpath = RT_NULL;
    struct dfs_mnt *mnt = RT_NULL;
    struct dfs_dentry *dentry = RT_NULL;

    fullpath = dfs_normalize_path(NULL, path);
    if (fullpath)
    {
        mnt = dfs_mnt_lookup(fullpath);
        if (mnt)
        {
            tmp = dfs_nolink_path(&mnt, fullpath, 0);
            if (tmp)
            {
                rt_free(fullpath);
                fullpath = tmp;
            }

            dentry = dfs_dentry_lookup(mnt, fullpath, O_CREAT | O_RDWR);
            if (dentry)
            {
                dfs_dentry_unref(dentry);
                ret = -EEXIST;
                goto out;
            }
            else
            {
                if (flag)
                {
                    fd = open(fullpath, O_CREAT, 0666);
                    if (fd >= 0)
                    {
                        close(fd);
                    }
                    else
                    {
                        ret = fd;
                        goto out;
                    }
                }

                ret = 0;
                dfs_file_lock();
                dentry = dfs_dentry_create(mnt, fullpath);
                dentry->vnode = df->vnode;
                dfs_dentry_insert(dentry);
                df->dentry = dentry;
                dfs_file_unlock();
            }
        }
    }

out:
    if (tmp)
        rt_free(tmp);

    return ret;
}

static int unix_bind_bsd(int fd, struct sockaddr_un *un_addr, int addr_len)
{
    rt_err_t ret = 0;
    struct dfs_file *df = RT_NULL;
    struct unix_sock *sock = RT_NULL;

    ret = -EBADF;
    df = fd_get(fd);
    if (!df)
        goto out;

    ret = -ENOTSOCK;
    if (!df->vnode)
        goto out;

    ret = -EINVAL;
    sock = df->vnode->data;
    if (!sock)
        goto out;

    ret = unix_do_bind(df, un_addr->sun_path, RT_TRUE);

    if (ret >= 0)
    {
        rt_mutex_take(&(sock->sock_lock), RT_WAITING_FOREVER);
        memset(sock->path, 0, sizeof(un_addr->sun_path));
        memcpy(sock->path, un_addr->sun_path, sizeof(un_addr->sun_path));
        rt_mutex_release(&(sock->sock_lock));
    }

out:

    return ret;
}

static int unix_bind_abstract(int fd, struct sockaddr_un *un_addr, int addr_len)
{
    rt_err_t ret = 0;
    struct dfs_file *df = RT_NULL;
    struct unix_sock *sock = RT_NULL;

    ret = -EBADF;
    df = fd_get(fd);
    if (!df)
        goto out;

    ret = -ENOTSOCK;
    if (!df->vnode)
        goto out;

    ret = -EINVAL;
    sock = df->vnode->data;
    if (!sock)
        goto out;

    ret = unix_do_bind(df, un_addr->sun_path + 1, RT_FALSE);

    if (ret >= 0)
    {
        rt_mutex_take(&(sock->sock_lock), RT_WAITING_FOREVER);
        memset(sock->path, 0, sizeof(un_addr->sun_path));
        memcpy(sock->path, un_addr->sun_path, sizeof(un_addr->sun_path));
        rt_mutex_release(&(sock->sock_lock));
    }

out:

    return ret;
}

int unix_bind(int s, const struct sockaddr *name, socklen_t namelen)
{
    rt_err_t ret = -EINVAL;
    struct sockaddr_un *un_addr = RT_NULL;

    if (!name)
        goto out;

    un_addr = (struct sockaddr_un *)name;

    if (unix_validate_addr(un_addr, namelen) < 0)
        goto out;

    if (un_addr->sa_family != AF_UNIX)
        goto out;

    if (un_addr->sun_path[0])
    {
        ret = unix_bind_bsd(s, un_addr, namelen);
    }
    else
    {
        ret = unix_bind_abstract(s, un_addr, namelen);
    }

out:
    if (ret < 0)
    {
        rt_set_errno(-ret);
        ret = -1;
    }

    return ret;
}

int unix_listen(int s, int backlog)
{
    rt_err_t ret = -EINVAL;
    struct dfs_file *df = RT_NULL;
    struct unix_sock *sock = RT_NULL;
    struct unix_conn *conn = RT_NULL;

    if ((backlog <= 0) || backlog > LISTTEN_MAX_NUM)
        goto out;

    ret = -EBADF;
    df = fd_get(s);
    if (!df)
        goto out;

    ret = -ENOTSOCK;
    if (!df->vnode)
        goto out;

    sock = df->vnode->data;
    if (!sock)
        goto out;

    ret = 0;
    conn = sock->conn;
    RT_ASSERT(conn != RT_NULL);

    rt_atomic_store(&(sock->listen_num), backlog);

    rt_mutex_take(&(conn->conn_lock), RT_WAITING_FOREVER);

    if ((conn->state & CONN_NONE) && !(conn->state & CONN_LISTEN))
        goto out_unlink;

    conn->state |= CONN_LISTEN;

out_unlink:
    rt_mutex_release(&(conn->conn_lock));

out:
    if (ret < 0)
    {
        rt_set_errno(-ret);
        ret = -1;
    }

    return ret;
}

static struct dfs_dentry *_get_dentry(char *path)
{
    char *tmp = RT_NULL;
    char *fullpath = RT_NULL;
    struct dfs_mnt *mnt = RT_NULL;
    struct dfs_dentry *dentry = RT_NULL;

    fullpath = dfs_normalize_path(NULL, path);

    if (fullpath)
    {
        mnt = dfs_mnt_lookup(fullpath);
        if (mnt)
        {
            tmp = dfs_nolink_path(&mnt, fullpath, 0);
            if (tmp)
            {
                rt_free(fullpath);
                fullpath = tmp;
            }

            dentry = dfs_dentry_lookup(mnt, fullpath, O_CREAT | O_RDWR);
        }
    }

    if (fullpath)
        rt_free(fullpath);

    return dentry;
}

static struct unix_sock *_get_sock(struct sockaddr_un *un_addr)
{
    struct unix_sock *us = 0;
    struct dfs_dentry *dentry = RT_NULL;

    if (un_addr->sun_path[0])
    {
        dentry = _get_dentry(un_addr->sun_path);
    }
    else
    {
        dentry = _get_dentry(un_addr->sun_path + 1);
    }

    if (dentry)
    {
        if (dentry->vnode)
        {
            us = dentry->vnode->data;
        }
        dfs_dentry_unref(dentry);
    }

    return us;
}

int unix_connect(int s, const struct sockaddr *name, socklen_t namelen)
{
    rt_err_t ret = -1;
    struct sockaddr_un *un_addr = RT_NULL;
    struct dfs_file *df = RT_NULL;
    struct unix_sock *sock = RT_NULL;
    struct unix_sock *ser_sock = RT_NULL;
    struct unix_conn *conn = RT_NULL;
    struct unix_conn *new_conn = RT_NULL;

    un_addr = (struct sockaddr_un *)name;

    ret = -EINVAL;
    if (unix_validate_addr(un_addr, namelen) < 0)
    {
        goto out;
    }

    ret = -EBADF;
    df = fd_get(s);
    if (!df)
        goto out;

    ser_sock = _get_sock(un_addr);

    ret = -EINVAL;
    if (!ser_sock)
        goto out;

    if (!df->vnode)
        goto out;

    sock = df->vnode->data;
    if (!sock)
        goto out;

    conn = sock->conn;
    RT_ASSERT(conn != RT_NULL);

    ret = -ENOMEM;
    new_conn = create_conn(AF_UNIX, conn->type, 0);
    if (!new_conn)
        goto out;

    ret = -ECONNREFUSED;
    if (ser_sock->conn)
    {
        if (!(ser_sock->conn->state & CONN_LISTEN))
        {
            goto fail_out;
        }
    }
    else
    {
        goto fail_out;
    }

    if ((rt_atomic_load(&(ser_sock->conn_counter)) + 1) > rt_atomic_load(&(ser_sock->listen_num)))
    {
        goto fail_out;
    }
    else
    {
        rt_mutex_take(&(conn->conn_lock), RT_WAITING_FOREVER);
        conn->correl_conn = new_conn;
        new_conn->correl_conn = conn;
        rt_mutex_release(&(conn->conn_lock));

        rt_mutex_take(&(ser_sock->sock_lock), RT_WAITING_FOREVER);

        rt_slist_append(&(ser_sock->wait_conn_head), &(conn->conn_node));
        rt_wqueue_wakeup(&(ser_sock->wq_head), (void*)POLLIN);

        rt_mutex_release(&(ser_sock->sock_lock));

        ret = 0;
        if (sock->flags & O_NONBLOCK)
        {
            goto out;
        }
        else
        {
            while(!(conn->state & CONN_CONNECT) || (conn->state & CONN_CLOSE))
            {
                rt_wqueue_wait(&(sock->wq_head), 0, RT_WAITING_FOREVER);
            }

            if (conn->state & CONN_CLOSE)
            {
                ret = -1;
            }
        }
    }

    goto out;

fail_out:
    if (new_conn)
    {
        if (new_conn->sock)
            rt_free(new_conn->sock);

        conn_release(new_conn);
    }

    if (ret < 0)
    {
        rt_set_errno(-ret);
        ret = -1;
    }

out:

    return ret;
}

int unix_accept(int s, struct sockaddr *addr, socklen_t *addrlen)
{
    int fd;
    int len = 0;
    rt_err_t ret = -1;
    rt_slist_t *node = RT_NULL;
    struct dfs_file *df = RT_NULL;
    struct dfs_file *new_df = RT_NULL;
    struct unix_sock *sock = RT_NULL;
    struct unix_conn *conn = RT_NULL;
    struct unix_conn *wait_conn = RT_NULL;
    struct unix_conn *new_conn = RT_NULL;
    struct sockaddr_un *addr_un = RT_NULL;

    df = fd_get(s);

    ret = -EINVAL;
    if (!df || !df->vnode)
    {
        goto out;
    }

    sock = df->vnode->data;
    if (!sock)
    {
        goto out;
    }

    addr_un = (struct sockaddr_un *)addr;

    conn = sock->conn;
    RT_ASSERT(conn != RT_NULL);

    ret = -EAGAIN;
    if ((sock->flags & O_NONBLOCK) && rt_slist_isempty(&(sock->wait_conn_head)))
        goto out;

    ret = -EINVAL;
    if (!(conn->state & CONN_LISTEN))
        goto out;

    fd = fd_new();
    if (fd <= 0)
    {
        ret = fd;
        goto out;
    }

    ret = -EBADF;
    new_df = fd_get(fd);
    if (!new_df)
    {
        goto df_out;
    }

    ret = -ENOMEM;
    new_df->vnode = (struct dfs_vnode *)rt_calloc(1, sizeof(struct dfs_vnode));
    if (!df->vnode)
    {
        goto df_out;
    }

    while (rt_slist_isempty(&(sock->wait_conn_head)))
    {
        rt_wqueue_wait(&(sock->wq_head), 0, RT_WAITING_FOREVER);
    }

    rt_mutex_take(&(sock->sock_lock), RT_WAITING_FOREVER);

    node = rt_slist_first(&(sock->wait_conn_head));
    rt_slist_remove(&(sock->wait_conn_head), node);

    rt_mutex_release(&(sock->sock_lock));

    wait_conn = rt_slist_entry(node, struct unix_conn, conn_node);
    new_conn = wait_conn->correl_conn;
    wait_conn->conn_callback(wait_conn);

    if (addr_un)
    {
        len = rt_strlen(wait_conn->sock->path);
        rt_strncpy(addr_un->sun_path, wait_conn->sock->path, len);
    }

    new_df->vnode->data = new_conn->sock;
    unix_sock_init(new_conn->sock, sock->family);
    memcpy(new_conn->sock->path, sock->path, sizeof(sock->path));

    new_conn->ser_sock = sock;

    rt_atomic_add(&(sock->conn_counter), 1);

    return fd;

df_out:
    fd_release(fd);

out:
    if (ret < 0)
    {
        rt_set_errno(-ret);
        ret = -1;
    }

    return -1;
}

static int unix_stream_sendto(struct unix_conn *conn, const void *data, size_t size, int flags)
{
    char *buf = RT_NULL;
    struct unix_conn *correl_conn = RT_NULL;
    struct msg_buf *msg = RT_NULL;

    if ((conn->state & CONN_SHUT_WR) || (conn->state & CONN_SHUT_RDWR))
        return -EPIPE;

    correl_conn = conn->correl_conn;

    if (!correl_conn)
        return -ENOTCONN;

    buf = rt_calloc(1, size);
    if (!buf)
        return -ENOMEM;

    msg = create_msgbuf();
    if (!msg)
        return -ENOMEM;

    memcpy(buf, data, size);

    msg->buf = buf;
    msg->length = size;

    rt_mutex_take(&(correl_conn->conn_lock), RT_WAITING_FOREVER);
    rt_slist_append(&(correl_conn->msg_head), &(msg->msg_node));
    rt_wqueue_wakeup(&(correl_conn->wq_read_head), (void *)POLLIN);
    rt_mutex_release(&(correl_conn->conn_lock));

    if (flags & MSG_CONFIRM)
    {
        conn->sock->flags |= MSG_CONFIRM;
        rt_wqueue_wait(&(conn->wq_confirm), 0, RT_WAITING_FOREVER);
    }
    else
    {
        conn->sock->flags &= ~MSG_CONFIRM;
    }

    return size;
}

static int unix_datagram_sendto(struct unix_conn *conn, const void *data, size_t size, int flags, const struct sockaddr *to, socklen_t tolen)
{
    char *buf = RT_NULL;
    struct msg_buf *msg = RT_NULL;
    struct sockaddr_un *un_addr = RT_NULL;
    struct unix_sock *recv_sock = RT_NULL;
    struct unix_conn *recv_conn = RT_NULL;

    un_addr = (struct sockaddr_un *)to;

    if (!un_addr)
        return -EDESTADDRREQ;

    recv_sock = _get_sock(un_addr);
    if (!recv_sock)
        return -EDESTADDRREQ;

    if ((conn->state & CONN_SHUT_WR) || (conn->state & CONN_SHUT_RDWR))
        return -EPIPE;

    buf = rt_calloc(1, size);
    if (!buf)
        return -ENOMEM;

    msg = create_msgbuf();
    if (!msg)
        return -ENOMEM;

    memcpy(buf, data, size);

    msg->buf = buf;
    msg->length = size;

    recv_conn = recv_sock->conn;
    RT_ASSERT(recv_conn != RT_NULL);

    rt_mutex_take(&(recv_conn->conn_lock), RT_WAITING_FOREVER);
    rt_slist_append(&(recv_conn->msg_head), &(msg->msg_node));
    rt_wqueue_wakeup(&(recv_conn->wq_read_head), (void *)POLLIN);
    rt_mutex_release(&(recv_conn->conn_lock));

    return size;
}

int unix_sendto(int s, const void *data, size_t size, int flags, const struct sockaddr *to, socklen_t tolen)
{
    rt_err_t ret = -EINVAL;
    struct dfs_file *df = RT_NULL;
    struct unix_sock *sock = RT_NULL;
    struct unix_conn *conn = RT_NULL;

    ret = -EBADF;
    df = fd_get(s);
    if (!df)
        goto out;

    ret = -ENOTSOCK;
    if (!df->vnode)
        goto out;

    sock = df->vnode->data;
    if (!sock)
        goto out;

    conn = sock->conn;
    RT_ASSERT(conn != RT_NULL);

    if (conn->state & CONN_CONNECT)
    {
        ret = unix_stream_sendto(conn, data, size, flags);
    }
    else
    {
        ret = unix_datagram_sendto(conn, data, size, flags, to, tolen);
    }

out:
    if (ret < 0)
    {
        rt_set_errno(-ret);
        ret = -1;
    }

    return ret;
}

static int copy_pbuf_data(void *ker_buf, size_t *buf_len, size_t *offset, void *user_buf, size_t user_len)
{
    int len = 0;

    if (*buf_len > user_len)
    {
        len = user_len;
        memcpy(user_buf, ker_buf + *offset, len);
        *offset = *offset + len;
        *buf_len = *buf_len - len;
    }
    else
    {
        len = *buf_len;
        memcpy(user_buf, ker_buf + *offset, len);
        *offset = 0;
        *buf_len = 0;
    }

    return len;
}

int unix_recvfrom(int s, void *mem, size_t len, int flags, struct sockaddr *from, socklen_t *fromlen)
{
    rt_err_t ret = -EINVAL;
    rt_slist_t *node = RT_NULL;
    struct dfs_file *df = RT_NULL;
    struct unix_sock *sock = RT_NULL;
    struct unix_conn *conn = RT_NULL;
    struct msg_buf *msg = RT_NULL;
    struct unix_conn *correl_conn = RT_NULL;

    ret = -EBADF;
    df = fd_get(s);
    if (!df)
        goto out;

    ret = -ENOTSOCK;
    if (!df->vnode)
        goto out;

    sock = df->vnode->data;

    if (!sock)
        goto out;

    conn = sock->conn;
    RT_ASSERT(conn != RT_NULL);

    ret = -EPIPE;
    if ((conn->state & CONN_SHUT_RD) || (conn->state & CONN_SHUT_RDWR))
        goto out;

    ret = 0;
    if((conn->state & CONN_CLOSE) && rt_slist_isempty(&(conn->msg_head)))
        goto out;

    rt_mutex_take(&(sock->sock_lock), RT_WAITING_FOREVER);

    if (sock->pbuf.buf != RT_NULL)
    {
        ret = copy_pbuf_data(sock->pbuf.buf, &(sock->pbuf.length), &(sock->pbuf.offset), mem, len);
        if (sock->pbuf.length == 0)
        {
            rt_free(sock->pbuf.buf);
            sock->pbuf.buf = RT_NULL;
        }
    }

    rt_mutex_release(&(sock->sock_lock));

    if (ret > 0)
        goto out;

    if ((sock->flags & O_NONBLOCK) && rt_slist_isempty(&(conn->msg_head)))
    {
        ret = -EAGAIN;
        goto out;
    }

    while(rt_slist_isempty(&(conn->msg_head)))
    {
        rt_wqueue_wait(&(conn->wq_read_head), 0, RT_WAITING_FOREVER);
    }

    rt_mutex_take(&(conn->conn_lock), RT_WAITING_FOREVER);
    node = rt_slist_first(&(conn->msg_head));
    rt_slist_remove(&(conn->msg_head), node);
    node->next = RT_NULL;
    msg = rt_slist_entry(node, struct msg_buf, msg_node);
    rt_mutex_release(&(conn->conn_lock));
    if (msg->length > len)
    {
        ret = len;
        memcpy(mem, msg->buf, len);

        rt_mutex_take(&(sock->sock_lock), RT_WAITING_FOREVER);
        sock->pbuf.length = msg->length - len;
        sock->pbuf.buf = msg->buf;
        msg->buf = RT_NULL;
        sock->pbuf.offset = sock->pbuf.offset + len;
        rt_mutex_release(&(sock->sock_lock));

        rt_free(msg);
    }
    else
    {
        ret = msg->length;
        memcpy(mem, msg->buf, msg->length);
        rt_free(msg->buf);
        msg->buf = RT_NULL;
        rt_free(msg);
    }

    msg = RT_NULL;

    correl_conn = conn->correl_conn;
    if (correl_conn)
    {
        rt_mutex_take(&(correl_conn->conn_lock), RT_WAITING_FOREVER);
        if (correl_conn->sock)
        {
            if (correl_conn->sock->flags & MSG_CONFIRM)
            {
                rt_wqueue_wakeup(&(correl_conn->wq_confirm), (void *)POLLIN);
            }
        }
        rt_mutex_release(&(correl_conn->conn_lock));
    }

out:
    if (ret < 0)
    {
        rt_set_errno(-ret);
        ret = -1;
    }

    return ret;
}

static int *_fd_news(rt_size_t cmsg_len, int *fd)
{
    int fd_num = 0;
    int *fd_array = RT_NULL;

    fd_num = (cmsg_len - CMSG_LEN(0))/sizeof(int);
    fd_array = rt_calloc(1, sizeof(int) * fd_num);
    if (fd_array)
    {
        for (int i = 0; i < fd_num; i ++)
        {
            fd_array[i] = dfs_dup_from(fd[i], RT_NULL);
        }
    }

    return fd_array;
}

static int _get_fds(struct cmsghdr *cmsg, int fd_num, int *fd)
{
    int *fd_array = (int *)CMSG_DATA(cmsg);

    for (int i = 0; i < fd_num; i ++)
    {
        fd[i] = dfs_dup_to(fd_array[i], RT_NULL);
    }

    return 0;
}

static int sendmsg_do(const struct msghdr *message, struct unix_conn *conn)
{
    rt_err_t ret = -ENOMEM;
    int msg_size = 0;
    int fd_num = 0;
    char *buf = RT_NULL;
    rt_size_t iovlen = 0;
    rt_size_t controllen = 0;
    struct iovec *iov = RT_NULL;
    struct msg_buf *msg = RT_NULL;
    struct msg_buf *msg_next = RT_NULL;
    struct cmsghdr *cmsg = RT_NULL;

    msg = create_msgbuf();

    if (!msg)
        goto out;

    iovlen = message->msg_iovlen;
    controllen = message->msg_controllen;

    if (iovlen != 0)
    {
        iov = message->msg_iov;
        if (iov != RT_NULL)
        {
            for (int i = 0; i < iovlen; i++)
            {
                if (i == 0)
                {
                    buf = rt_calloc(1, iov[i].iov_len);
                    if (!buf)
                        goto out;

                    memcpy(buf, iov[i].iov_base, iov[i].iov_len);
                    msg->length = iov[i].iov_len;
                    msg->buf = buf;
                }
                else
                {
                    buf = rt_calloc(1, iov[i].iov_len);
                    if (!buf)
                        goto out;

                    msg_next = create_msgbuf();
                    if (!msg_next)
                    {
                        rt_free(buf);
                        goto out;
                    }
                    rt_slist_append(&(msg->msg_next), &(msg_next->msg_next));
                    memcpy(buf, iov[i].iov_base, iov[i].iov_len);
                    msg_next->length = iov[i].iov_len;
                    msg_next->buf = buf;
                }
                msg_size = iov[i].iov_len + msg_size;
            }
        }
    }

    if (controllen != 0)
    {
        cmsg = message->msg_control;
        if (cmsg != RT_NULL)
        {
            if (cmsg->cmsg_type & SCM_RIGHTS)
            {
                fd_num = (cmsg->cmsg_len - CMSG_LEN(0))/sizeof(int);
                msg->fd = rt_calloc(1, sizeof(int) * fd_num);
                if (msg->fd)
                {
                    ret = _get_fds(cmsg, fd_num, msg->fd);
                }
                else
                {
                    goto out;
                }
            }
            else
            {
                buf = rt_calloc(1, cmsg->cmsg_len);
                if (!buf)
                    goto out;

                memcpy(buf, (void *)CMSG_DATA(cmsg), cmsg->cmsg_len);
                msg->control_data = buf;
            }

            msg->data_type = cmsg->cmsg_type;
            msg->msg_level = cmsg->cmsg_level;
            msg->data_len = cmsg->cmsg_len;
        }
    }

    rt_slist_append(&(conn->msg_head), &(msg->msg_node));
    rt_wqueue_wakeup(&(conn->wq_read_head), (void *)POLLIN);

    return msg_size;

out:
    if (ret < 0)
        rt_set_errno(-ret);

    msg_buf_release(msg);
    msg = RT_NULL;

    return ret;
}

static int unix_stream_sendmsg(struct unix_conn *conn, const struct msghdr *message, int flags)
{
    int size = 0;
    struct unix_conn *correl_conn = RT_NULL;

    rt_mutex_take(&(conn->conn_lock), RT_WAITING_FOREVER);
    if ((conn->state & CONN_SHUT_WR) || (conn->state & CONN_SHUT_RDWR))
    {
        rt_mutex_release(&(conn->conn_lock));
        return -EPIPE;
    }

    correl_conn = conn->correl_conn;
    rt_mutex_release(&(conn->conn_lock));

    if (!correl_conn)
        return -ENOTCONN;

    if (flags & MSG_CONFIRM)
    {
        rt_mutex_take(&(correl_conn->conn_lock), RT_WAITING_FOREVER);
        conn->sock->flags |= MSG_CONFIRM;
        size = sendmsg_do(message, correl_conn);
        rt_mutex_release(&(correl_conn->conn_lock));
        if (conn && (size >=0 ))
        {
            rt_wqueue_wait(&(conn->wq_confirm), 0, RT_WAITING_FOREVER);
        }
    }
    else
    {
        rt_mutex_take(&(correl_conn->conn_lock), RT_WAITING_FOREVER);
        conn->sock->flags &= ~MSG_CONFIRM;
        size = sendmsg_do(message, correl_conn);
        rt_mutex_release(&(correl_conn->conn_lock));
    }

    return size;
}

static int unix_datagram_sendmsg(struct unix_conn *conn, const struct msghdr *message)
{
    int size = 0;
    struct sockaddr_un *un_addr = RT_NULL;
    struct unix_sock *recv_sock = RT_NULL;
    struct unix_conn *recv_conn = RT_NULL;

    if (!message)
        return -EDESTADDRREQ;

    un_addr = (struct sockaddr_un *)message->msg_name;

    if (!un_addr)
        return -EDESTADDRREQ;

    recv_sock = _get_sock(un_addr);
    if (!recv_sock)
        return -EDESTADDRREQ;

    recv_conn = recv_sock->conn;
    RT_ASSERT(recv_conn != RT_NULL);

    if ((conn->state & CONN_SHUT_WR) || (conn->state & CONN_SHUT_RDWR))
        return -EPIPE;

    rt_mutex_take(&(recv_conn->conn_lock), RT_WAITING_FOREVER);
    size = sendmsg_do(message, recv_conn);
    rt_mutex_release(&(recv_conn->conn_lock));

    return size;
}

int unix_sendmsg(int s, const struct msghdr *message, int flags)
{
    rt_err_t ret = -EINVAL;
    struct dfs_file *df = RT_NULL;
    struct unix_sock *sock = RT_NULL;
    struct unix_conn *conn = RT_NULL;

    ret = -EBADF;
    df = fd_get(s);
    if (!df)
        goto out;

    ret = -ENOTSOCK;
    if (!df->vnode)
        goto out;

    sock = df->vnode->data;

    if(!sock)
        goto out;

    conn = sock->conn;
    RT_ASSERT(conn != RT_NULL);

    if (conn->state & CONN_CONNECT)
    {
        ret = unix_stream_sendmsg(conn, message, flags);
    }
    else
    {
        ret = unix_datagram_sendmsg(conn, message);
    }

out:
    if (ret < 0)
    {
        rt_set_errno(-ret);
        ret = -1;
    }

    return ret;
}

static int copy_msg_data(struct unix_sock *sock, struct unix_conn *conn, struct iovec *iov, int index, rt_size_t iovlen, struct msg_buf *msg)
{
    rt_size_t size = 0;
    rt_size_t msg_size = 0;
    rt_slist_t *node = RT_NULL;

    if ((iovlen != 0) && (iov != RT_NULL) && (msg->buf != RT_NULL))
    {
        for (; index < iovlen; index ++)
        {
            if (msg->length > iov[index].iov_len)
            {
                memcpy(iov[index].iov_base, msg->buf + size, iov[index].iov_len);
                size = size + iov[index].iov_len;
                msg_size = msg_size + iov[index].iov_len;
                msg->length = msg->length - iov[index].iov_len;
            }
            else
            {
                size = 0;
                msg_size = msg_size + msg->length;
                memcpy(iov[index].iov_base, msg->buf, msg->length);
                if (rt_slist_isempty(&(msg->msg_next)))
                {
                    msg_buf_release(msg);
                    msg = RT_NULL;
                    break;
                }

                node = rt_slist_first(&(msg->msg_next));
                rt_slist_remove(&(msg->msg_next), node);
                _msg_release(msg);
                msg = RT_NULL;
                msg = rt_slist_entry(node, struct msg_buf, msg_next);
            }
        }
    }

    if ((index >= iovlen) && (msg != RT_NULL))
    {
        if (sock->pbuf.buf == RT_NULL)
        {
            sock->pbuf.length = msg->length;
            sock->pbuf.buf = msg->buf;
            if (sock->pbuf.msg)
            {
                _msg_release(sock->pbuf.msg);
            }
            sock->pbuf.msg = msg;
            sock->pbuf.offset = sock->pbuf.offset + size;
            msg->buf = RT_NULL;
        }
        else
        {
            if (conn)
            {
                rt_mutex_take(&(conn->conn_lock), RT_WAITING_FOREVER);
                rt_slist_insert(&(conn->msg_head), &(msg->msg_node));
                rt_mutex_release(&(conn->conn_lock));
            }
        }
    }

    return msg_size;
}

static int unix_stream_recvmsg(struct unix_sock *sock, const struct msghdr *message)
{
    int index = 0;
    rt_size_t size = 0;
    rt_size_t iovlen = 0;
    rt_size_t msg_size = 0;
    rt_size_t controllen = 0;
    int *fd_array = RT_NULL;
    rt_slist_t *node = RT_NULL;
    struct iovec *iov = RT_NULL;
    struct cmsghdr *cmsg = RT_NULL;
    struct msg_buf *msg = RT_NULL;
    struct unix_conn *conn = RT_NULL;
    struct unix_conn *correl_conn = RT_NULL;

    iovlen = message->msg_iovlen;
    controllen = message->msg_controllen;
    if ((iovlen == 0) && (controllen == 0))
        return -EINVAL;

    iov = message->msg_iov;
    cmsg = message->msg_control;

    if ((iovlen != 0) && (!iov))
    {
        return -EINVAL;
    }

    if ((controllen != 0) && (!cmsg))
    {
        return -EINVAL;
    }

    conn = sock->conn;
    RT_ASSERT(conn != RT_NULL);

    rt_mutex_take(&(conn->conn_lock), RT_WAITING_FOREVER);

    if ((conn->state & CONN_SHUT_RD) || (conn->state & CONN_SHUT_RDWR))
    {
        rt_mutex_release(&(conn->conn_lock));
        return -EPIPE;
    }
    else if((conn->state & CONN_CLOSE) && rt_slist_isempty(&(conn->msg_head)))
    {
        rt_mutex_release(&(conn->conn_lock));
        return 0;
    }

    rt_mutex_release(&(conn->conn_lock));

    rt_mutex_take(&(sock->sock_lock), RT_WAITING_FOREVER);
    if ((iovlen != 0) && ((sock->pbuf.buf) || (sock->pbuf.msg)))
    {
        if (!iov)
        {
            rt_mutex_release(&(sock->sock_lock));
            return -EINVAL;
        }

        if (sock->pbuf.buf)
        {
            for (; index < iovlen; index ++)
            {
                size = copy_pbuf_data(sock->pbuf.buf, &(sock->pbuf.length), &(sock->pbuf.offset), iov[index].iov_base, iov[index].iov_len);
                msg_size = msg_size + size;

                if (sock->pbuf.length == 0)
                {
                    rt_free(sock->pbuf.buf);
                    sock->pbuf.buf = RT_NULL;
                    sock->pbuf.offset = 0;
                    index ++;
                    break;
                }
            }
        }

        if ((sock->pbuf.msg) && (index < iovlen))
        {
            if(!rt_slist_isempty(&(sock->pbuf.msg->msg_next)))
            {
                node = rt_slist_first(&(sock->pbuf.msg->msg_next));
                rt_slist_remove(&(sock->pbuf.msg->msg_next), node);
                _msg_release(sock->pbuf.msg);
                sock->pbuf.msg = RT_NULL;
                msg = rt_slist_entry(node, struct msg_buf, msg_next);
                size = copy_msg_data(sock, conn, iov, index, iovlen, msg);
                msg_size = msg_size + size;
            }
            else
            {
                _msg_release(sock->pbuf.msg);
                sock->pbuf.msg = RT_NULL;
            }
        }
    }

    rt_mutex_release(&(sock->sock_lock));

    if ((sock->flags & O_NONBLOCK) && rt_slist_isempty(&(conn->msg_head)) && (msg_size == 0))
    {
        return -EAGAIN;
    }

    if (rt_slist_isempty(&(conn->msg_head)) && (msg_size > 0))
    {
        return msg_size;
    }

    while(rt_slist_isempty(&(conn->msg_head)))
    {
        rt_wqueue_wait(&(conn->wq_read_head), 0, RT_WAITING_FOREVER);
    }

    rt_mutex_take(&(conn->conn_lock), RT_WAITING_FOREVER);

    node = rt_slist_first(&(conn->msg_head));
    rt_slist_remove(&(conn->msg_head), node);
    node->next = RT_NULL;
    msg = rt_slist_entry(node, struct msg_buf, msg_node);
    rt_mutex_release(&(conn->conn_lock));

    if ((controllen != 0))
    {
        if (cmsg)
        {
            if (msg->data_type == SCM_RIGHTS)
            {
                if (msg->data_len > 0)
                {
                    fd_array = _fd_news(msg->data_len, msg->fd);
                    if (fd_array)
                    {
                        memcpy(CMSG_DATA(cmsg), (void *)fd_array, msg->data_len - CMSG_LEN(0));
                    }
                    else
                    {
                        _close_fd_news(msg->fd, ((msg->data_len - CMSG_LEN(0))/sizeof(int)));
                    }

                    if (msg->fd)
                    {
                        rt_free(msg->fd);
                        msg->fd = RT_NULL;
                    }
                }
            }
            else
            {
                if (msg->control_data)
                {
                    memcpy(CMSG_DATA(cmsg), msg->control_data, msg->data_len);
                }
            }

            cmsg->cmsg_len = msg->data_len;
            cmsg->cmsg_level = msg->msg_level;
            cmsg->cmsg_type = msg->data_type;
        }
    }
    else
    {
        if (msg->data_type == SCM_RIGHTS)
        {
            _close_fd_news(msg->fd, ((msg->data_len - CMSG_LEN(0))/sizeof(int)));
        }
    }

    if (msg->control_data)
    {
        rt_free(msg->control_data);
        msg->control_data = RT_NULL;
    }

    if (fd_array)
    {
        rt_free(fd_array);
        fd_array = RT_NULL;
    }

    msg->data_type = 0;
    msg->data_len = 0;

    msg_size += copy_msg_data(sock, conn, iov, index, iovlen, msg);

    correl_conn = conn->correl_conn;
    if (conn->correl_conn)
    {
        rt_mutex_take(&(correl_conn->conn_lock), RT_WAITING_FOREVER);
        if (correl_conn->sock)
        {
            if (correl_conn->sock->flags & MSG_CONFIRM)
            {
                rt_wqueue_wakeup(&(correl_conn->wq_confirm), (void *)POLLIN);
            }
        }
        rt_mutex_release(&(correl_conn->conn_lock));
    }

    return msg_size;
}

int unix_recvmsg(int s, struct msghdr *message, int flags)
{
    rt_err_t ret = -EINVAL;
    struct dfs_file *df = RT_NULL;
    struct unix_sock *sock = RT_NULL;

    ret = -EBADF;
    df = fd_get(s);
    if (!df)
        goto out;

    ret = -ENOTSOCK;
    if (!df->vnode)
        goto out;

    sock = df->vnode->data;
    if(!sock)
        goto out;

    ret = unix_stream_recvmsg(sock, message);

out:
    if (ret < 0)
    {
        rt_set_errno(-ret);
        ret = -1;
    }

    return ret;
}

int unix_getsockopt(int s, int level, int optname, void *optval, socklen_t *optlen)
{
    rt_err_t ret = 0;
    int *val = RT_NULL;
    struct dfs_file *df = RT_NULL;
    struct unix_sock *sock = RT_NULL;
    struct unix_conn *conn = RT_NULL;

    ret = -EBADF;
    df = fd_get(s);
    if (!df)
        goto out;

    ret = -ENOTSOCK;
    if (!df->vnode)
        goto out;

    sock = df->vnode->data;
    if(!sock)
        goto out;

    ret = -EOPNOTSUPP;
    if ((level & IPPROTO_IP) ||
        (level & IPPROTO_IPV6) ||
        (level & IPPROTO_TCP) ||
        (level & IPPROTO_UDP))
    {
        goto out;
    }

    ret = -EINVAL;
    if (!(level & SOL_SOCKET))
        goto out;

    rt_mutex_take(&(sock->sock_lock), RT_WAITING_FOREVER);

    conn = sock->conn;
    val = (int *)optval;
    ret = 0;
    switch (optname)
    {
        case SO_PROTOCOL:
        case SO_DOMAIN:
            *val = sock->family;
            break;
        case SO_TYPE:
            *val = conn->type;
            break;
        case SO_ERROR:
        case SO_PASSCRED:
        case SO_PEERCRED:
        case SO_ACCEPTCONN:
            *val = 0;
            break;
        case SO_BINDTODEVICE:
        case SO_RCVBUFFORCE:
        case SO_SNDBUFFORCE:
        case SO_ATTACH_FILTER:
        case SO_DETACH_FILTER:
            ret = -EOPNOTSUPP;
            break;
        case SO_RCVBUF:
            *val = UNIX_MSG_MAX_BYTE;
            break;
        case SO_SNDBUF:
            *val = UNIX_MSG_MAX_BYTE;
            break;
        default:
            ret = -EINVAL;
            break;
    }

    rt_mutex_release(&(sock->sock_lock));

out:
    if (ret < 0)
    {
        rt_set_errno(-ret);
        ret = -1;
    }

    return ret;
}

static int set_timeout(struct unix_conn *conn, const void *optval, socklen_t optlen, rt_bool_t is_send_timeout)
{
    rt_err_t ret = -EINVAL;
    struct timeval *timeout = RT_NULL;

    if (optlen < sizeof(struct timeval))
        goto out;

    timeout = (struct timeval *)optval;
    if (!timeout)
        goto out;

    ret = 0;
    if (is_send_timeout)
    {
        conn->send_timeout = (timeout->tv_usec / MSEC_TO_USEC) + (timeout->tv_sec * MSEC_TO_USEC);
    }
    else
    {
        conn->recv_timeout = (timeout->tv_usec / MSEC_TO_USEC) + (timeout->tv_sec * MSEC_TO_USEC);
    }

out:
    return ret;
}

int unix_setsockopt(int s, int level, int optname, const void *optval, socklen_t optlen)
{
    rt_err_t ret = 0;
    struct dfs_file *df = RT_NULL;
    struct unix_sock *sock = RT_NULL;
    struct unix_conn *conn = RT_NULL;

    ret = -EBADF;
    df = fd_get(s);
    if (!df)
        goto out;

    ret = -ENOTSOCK;
    if (!df->vnode)
        goto out;

    sock = df->vnode->data;
    if(!sock)
        goto out;

    ret = -EOPNOTSUPP;
    if ((level & IPPROTO_IP) ||
        (level & IPPROTO_IPV6) ||
        (level & IPPROTO_TCP) ||
        (level & IPPROTO_UDP))
    {
        goto out;
    }

    ret = -EINVAL;
    if (!(level & SOL_SOCKET))
        goto out;

    conn = sock->conn;
    rt_mutex_take(&(conn->conn_lock), RT_WAITING_FOREVER);
    ret = 0;
    switch (optname)
    {
        case SO_LINGER:
            break;
        case SO_REUSEADDR:
        case SO_KEEPALIVE:
        case SO_BROADCAST:
            ret = -EOPNOTSUPP;
            break;
        case SO_RCVBUF:
            // *optval = UNIX_MSG_MAX_BYTE;
            break;
        case SO_SNDBUF:
            // *optval = UNIX_MSG_MAX_BYTE;
            break;
        case SO_SNDTIMEO:
            ret = set_timeout(conn, optval, optlen, RT_TRUE);
            break;
        case SO_RCVTIMEO:
            ret = set_timeout(conn, optval, optlen, RT_FALSE);
            break;
        default:
            ret = -EINVAL;
            break;
    }

    rt_mutex_release(&(conn->conn_lock));

out:
    if (ret < 0)
    {
        rt_set_errno(-ret);
        ret = -1;
    }

    return ret;
}

int unix_shutdown(int s, int how)
{
    rt_err_t ret = -EINVAL;
    struct dfs_file *df = RT_NULL;
    struct unix_sock *sock = RT_NULL;
    struct unix_conn *conn = RT_NULL;

    ret = -EINVAL;
    if (how < SHUT_RD || how > SHUT_RDWR)
        goto out;

    ret = -EBADF;
    df = fd_get(s);
    if (!df)
        goto out;

    ret = -ENOTSOCK;
    if (!df->vnode)
        goto out;

    sock = df->vnode->data;

    ret = -ENOTSOCK;
    if (!sock)
        goto out;

    conn = sock->conn;
    RT_ASSERT(conn != RT_NULL);

    rt_mutex_take(&(conn->conn_lock), RT_WAITING_FOREVER);

    switch (how)
    {
        case SHUT_RD:
            conn->state |= CONN_SHUT_RD;
            break;
        case SHUT_WR:
            conn->state |= CONN_SHUT_WR;
            break;
        case SHUT_RDWR:
            conn->state |= CONN_SHUT_RDWR;
            break;
        default:
            break;
    }

    rt_mutex_release(&(conn->conn_lock));

    ret = 0;

out:
    if (ret < 0)
    {
        rt_set_errno(-ret);
        ret = -1;
    }

    return ret;
}
int unix_getpeername(int s, struct sockaddr *name, socklen_t *namelen)
{
    rt_err_t ret = -EINVAL;
    rt_size_t len = 0;
    struct dfs_file *df = RT_NULL;
    struct unix_sock *sock = RT_NULL;
    struct unix_conn *conn = RT_NULL;
    struct sockaddr_un *addr_un = RT_NULL;
    struct unix_conn *correl_conn = RT_NULL;

    addr_un = (struct sockaddr_un *)name;
    if (!addr_un)
        goto out;

    ret = -EBADF;
    df = fd_get(s);
    if (!df)
        goto out;

    ret = -ENOTSOCK;
    if (!df->vnode)
        goto out;

    sock = df->vnode->data;

    if (!sock)
        goto out;

    conn = sock->conn;
    RT_ASSERT(conn != RT_NULL);

    ret = -ENOTCONN;
    correl_conn = conn->correl_conn;
    if (correl_conn)
    {
        ret = 0;
        if (correl_conn->ser_sock->path[0])
        {
            len = rt_strlen(correl_conn->ser_sock->path);
            rt_strncpy(addr_un->sun_path, correl_conn->ser_sock->path, len);
        }
        else
        {
            len = rt_strlen(correl_conn->ser_sock->path + 1);
            rt_strncpy(addr_un->sun_path + 1, correl_conn->ser_sock->path + 1, len);
        }
    }

out:
    if (ret < 0)
    {
        rt_set_errno(-ret);
        ret = -1;
    }

    return ret;
}

int unix_getsockname(int s, struct sockaddr *name, socklen_t *namelen)
{
    rt_err_t ret = -EINVAL;
    struct dfs_file *df = RT_NULL;
    struct unix_sock *sock = RT_NULL;
    struct sockaddr_un *addr_un = RT_NULL;

    addr_un = (struct sockaddr_un *)name;
    if(!addr_un)
        goto out;

    ret = -EBADF;
    df = fd_get(s);
    if (!df)
        goto out;

    ret = -ENOTSOCK;
    if (!df->vnode)
        goto out;

    sock = df->vnode->data;

    ret = -ENOTSOCK;
    if (!sock)
        goto out;

    ret = rt_strlen(sock->path);
    if (ret > 0)
    {
        memcpy(addr_un->sun_path, sock->path, ret);
        *namelen = ret;
        ret = 0;
    }

out:
    if (ret < 0)
    {
        rt_set_errno(-ret);
        ret = -1;
    }

    return ret;
}

int unix_poll(struct dfs_file *file, struct rt_pollreq *req)
{
    int events = 0;
    struct unix_sock *sock = RT_NULL;
    struct unix_conn *conn = RT_NULL;

    if (!file->vnode)
    {
        return -1;
    }

    sock = file->vnode->data;
    if (!sock)
    {
        return -1;
    }

    conn = sock->conn;
    RT_ASSERT(conn != RT_NULL);

    rt_mutex_take(&(conn->conn_lock), RT_WAITING_FOREVER);
    if (conn->state & CONN_LISTEN)
    {
        rt_mutex_release(&(conn->conn_lock));

        rt_mutex_take(&(sock->sock_lock), RT_WAITING_FOREVER);
        rt_poll_add(&(sock->wq_head), req);

        if (!rt_slist_isempty(&(sock->wait_conn_head)))
        {
            events |= POLLIN;
        }

        rt_mutex_release(&(sock->sock_lock));
    }
    else
    {
        rt_poll_add(&(conn->wq_read_head), req);

        if (conn->state & CONN_CLOSE)
        {
            events |= POLLHUP;
        }

        if ((conn->state & CONN_CLOSE) || (conn->state & CONN_NONE))
        {
            events |= POLLERR;
        }

        rt_mutex_release(&(conn->conn_lock));

        rt_mutex_take(&(sock->sock_lock), RT_WAITING_FOREVER);
        if ((sock->pbuf.buf) || (sock->pbuf.msg))
        {
            events |= POLLIN;
        }

        rt_mutex_release(&(sock->sock_lock));
    }

    if (conn->state & CONN_CONNECT)
    {
        events |= POLLOUT | POLLWRNORM;
    }

    if (!rt_slist_isempty(&(conn->msg_head)))
    {
        events |= POLLIN;
    }

    return events;
}

static int unix_df_close(struct dfs_file *df)
{
    return 0;
}

int unix_close(int s)
{
    rt_err_t ret = 0;
    struct dfs_file *df = RT_NULL;
    struct unix_sock *sock = RT_NULL;
    struct unix_sock *ser_sock = RT_NULL;
    struct unix_conn *conn = RT_NULL;
    struct unix_conn *correl_conn = RT_NULL;
    df = fd_get(s);
    if (!df)
        goto out;

    if (!df->vnode)
        goto dfs_out;

    sock = df->vnode->data;
    if (!sock)
        goto dfs_out;

    conn = sock->conn;
    RT_ASSERT(conn != RT_NULL);

    rt_mutex_take(&(conn->conn_lock), RT_WAITING_FOREVER);

    correl_conn = conn->correl_conn;
    conn->correl_conn = RT_NULL;
    rt_mutex_release(&(conn->conn_lock));

    if (correl_conn)
    {
        rt_mutex_take(&(correl_conn->conn_lock), RT_WAITING_FOREVER);
        correl_conn->state = CONN_CLOSE;
        correl_conn->correl_conn = RT_NULL;
        ser_sock = correl_conn->ser_sock;
        rt_wqueue_wakeup(&(correl_conn->wq_read_head), (void *)POLLIN);
        rt_mutex_release(&(correl_conn->conn_lock));
    }

    sock_release(sock);

    if (ser_sock)
    {
        rt_atomic_sub(&(ser_sock->conn_counter), 1);
    }

dfs_out:
    close(s);

out:
    if (ret < 0)
    {
        rt_set_errno(-ret);
        ret = -1;
    }

    return ret;
}

static int add_netdev_lo(void)
{
#if defined(RT_USING_DFS_V2) && defined(SAL_USING_AF_UNIX)
    struct netdev *netdev = RT_NULL;
    netdev = rt_malloc(sizeof(struct netdev));
    memset(netdev, '\0', sizeof(struct netdev));
    netdev_register(netdev, "unix", RT_NULL);
    netdev->flags |= NETDEV_FLAG_UP;
    extern int sal_unix_netdev_set_pf_info(struct netdev *netdev);
    sal_unix_netdev_set_pf_info(netdev);
    netdev_lo = netdev;
    return 0;
#endif
}

INIT_COMPONENT_EXPORT(add_netdev_lo);