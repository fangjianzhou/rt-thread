#include <rtthread.h>

#ifdef SAL_USING_POSIX
#include <poll.h>
#endif

#include <sal_low_lvl.h>
#include <af_inet.h>
#include <af_unix.h>
#include <netdev.h>

static int inet_ioctlsocket(int socket, long cmd, void *arg)
{
    rt_set_errno(-EPERM);

    return -1;
}

#ifdef SAL_USING_POSIX
static int inet_poll(struct dfs_file *file, struct rt_pollreq *req)
{
    int events = 0;
    struct dfs_file *df = RT_NULL;
    struct sal_socket *sal_sock = RT_NULL;

    sal_sock = sal_get_socket((int)(size_t)file->vnode->data);
    if(!sal_sock)
    {
        return -1;
    }

    df = fd_get((int)(size_t)sal_sock->user_data);
    if (!df)
    {
        return -1;
    }

    events = unix_poll(df, req);

    return events;
}
#endif

static const struct sal_socket_ops unix_socket_ops =
{
    .socket      = unix_socket,
    .closesocket = unix_close,
    .bind        = unix_bind,
    .listen      = unix_listen,
    .connect     = unix_connect,
    .accept      = unix_accept,
    .sendto      = (int (*)(int, const void *, size_t, int, const struct sockaddr *, socklen_t))unix_sendto,
    .sendmsg     = (int (*)(int, const struct msghdr *, int))unix_sendmsg,
    .recvmsg     = (int (*)(int, struct msghdr *, int))unix_recvmsg,
    .recvfrom    = (int (*)(int, void *, size_t, int, struct sockaddr *, socklen_t *))unix_recvfrom,
    .getsockopt  = unix_getsockopt,
    .setsockopt  = unix_setsockopt,
    .shutdown    = unix_shutdown,
    .getpeername = unix_getpeername,
    .getsockname = unix_getsockname,
    .ioctlsocket = inet_ioctlsocket,
#ifdef SAL_USING_POSIX
    .poll        = inet_poll,
#endif
};

static const struct sal_proto_family unix_inet_family =
{
    .family     = AF_UNIX,
    .sec_family = AF_UNIX,
    .skt_ops    = &unix_socket_ops,
    .netdb_ops  = RT_NULL,
};

/* Set unix network interface device protocol family information */
int sal_unix_netdev_set_pf_info(struct netdev *netdev)
{
    RT_ASSERT(netdev);

    netdev->sal_user_data = (void *) &unix_inet_family;
    return 0;
}