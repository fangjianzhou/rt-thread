/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2015-02-17     Bernard      First version
 * 2016-05-07     Bernard      Rename dfs_lwip to dfs_net
 * 2018-03-09     Bernard      Fix the last data issue in poll.
 * 2018-05-24     ChenYong     Add socket abstraction layer
 */

#include <rtthread.h>

#include <dfs.h>
#include <dfs_net.h>

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sal_low_lvl.h>

#ifdef RT_USING_SMART
#include <lwp_user_mm.h>
#include <lwp_sys_socket.h>
#endif

int dfs_net_getsocket(int fd)
{
    int socket;
    struct dfs_file *file;

    file = fd_get(fd);
    if (file == NULL) return -1;

    if (file->vnode->type != FT_SOCKET) socket = -1;
    else socket = (int)(size_t)file->vnode->data;

    return socket;
}

static int dfs_net_ioctl(struct dfs_file* file, int cmd, void* args)
{
    int ret = -1;
    int socket = (int)(size_t)file->vnode->data;

#ifdef RT_USING_SMART
    switch (cmd)
    {
    case SIOCADDRT:
    {
        struct sal_socket *sock = sal_get_socket(socket);
        if (sock && args)
        {
            struct musl_rtentry rt;

            if (!lwp_user_accessable((void *)args, sizeof(rt)))
            {
                rt_set_errno(-EFAULT);
                return ret;
            }

            lwp_get_from_user(&rt, args, sizeof(rt));

            if (rt.rt_dev && sock->domain == AF_INET)
            {
                extern int route_ipv4_add(char *ifname, char *ip_addr, char *nm_addr);
                return route_ipv4_add(rt.rt_dev, &(rt.rt_dst.sa_data[2]), &(rt.rt_genmask.sa_data[2]));
            }
        }
    }
    break;
    case SIOCDELRT:
    {
        struct sal_socket *sock = sal_get_socket(socket);
        if (sock && args)
        {
            struct musl_rtentry rt;

            if (!lwp_user_accessable((void *)args, sizeof(rt)))
            {
                rt_set_errno(-EFAULT);
                return ret;
            }

            lwp_get_from_user(&rt, args, sizeof(rt));

            if (rt.rt_dev && sock->domain == AF_INET)
            {
                extern int route_ipv4_delete(char *ip_addr, char *nm_addr);
                return route_ipv4_delete(&(rt.rt_dst.sa_data[2]), &(rt.rt_genmask.sa_data[2]));
            }
        }
    }
    break;
#ifdef  RT_LWIP_SUPPORT_VLAN
#define SIOCSIFVLAN           0x8983  /* Set 802.1Q VLAN options */
    case SIOCSIFVLAN:
    {
        extern int vlan_ioctl_handler(void *arg);
        if (args)
        {
            return vlan_ioctl_handler(args);
        }
    }
    break;
#endif
    }
#endif

    ret = sal_ioctlsocket(socket, cmd, args);
    if (ret < 0)
    {
        ret = rt_get_errno();
        return (ret > 0) ? (-ret) : ret;
    }
    return ret;
}

#ifdef RT_USING_DFS_V2
static ssize_t dfs_net_read(struct dfs_file* file, void *buf, size_t count, off_t *pos)
#else
static ssize_t dfs_net_read(struct dfs_file* file, void *buf, size_t count)
#endif
{
    int ret;
    int socket = (int)(size_t)file->vnode->data;

    ret = sal_recvfrom(socket, buf, count, 0, NULL, NULL);
    if (ret < 0)
    {
        ret = rt_get_errno();
        return (ret > 0) ? (-ret) : ret;
    }

    return ret;
}

#ifdef RT_USING_DFS_V2
static ssize_t dfs_net_write(struct dfs_file *file, const void *buf, size_t count, off_t *pos)
#else
static ssize_t dfs_net_write(struct dfs_file *file, const void *buf, size_t count)
#endif
{
    int ret;
    int socket = (int)(size_t)file->vnode->data;

    ret = sal_sendto(socket, buf, count, 0, NULL, 0);
    if (ret < 0)
    {
        ret = rt_get_errno();
        return (ret > 0) ? (-ret) : ret;
    }

    return ret;
}

static int dfs_net_close(struct dfs_file* file)
{
    int socket;
    int ret = 0;

    if (file->vnode->ref_count == 1)
    {
        socket = (int)(size_t)file->vnode->data;
        ret = sal_closesocket(socket);
    }
    return ret;
}

static int dfs_net_poll(struct dfs_file *file, struct rt_pollreq *req)
{
    extern int sal_poll(struct dfs_file *file, struct rt_pollreq *req);

    return sal_poll(file, req);
}

const struct dfs_file_ops _net_fops =
{
    .close = dfs_net_close,
    .ioctl = dfs_net_ioctl,
    .read  = dfs_net_read,
    .write = dfs_net_write,
    .poll  = dfs_net_poll,
};

const struct dfs_file_ops *dfs_net_get_fops(void)
{
    return &_net_fops;
}
