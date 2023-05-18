#include <rtthread.h>
#include <lwip/opt.h>

#ifdef LWIP_ROUTE    /* don't build if not configured for use in rtconfig.h */
#include <lwip/ip_route.h>

#define RTACTION_ADD 1   /* add action */
#define RTACTION_DEL 2   /* del action */

/* [LWIPROUTE_ROUTE_0515] */
#if LWIP_IPV4
/*********************************************************
**   function name:         route4_add
**   Descriptions:          设置一个naptpt接口
**   input parameters:      ifname: 指定网络接口
**                          ip_addr:添加的IP地址
**                          nm_addr:添加的子网掩码
**   outout parameters:     无
**   Returned value:        无
*********************************************************/
int route4_add(char* ifname, char* ip_addr, char* nm_addr)
{
    ip4_addr_t ip4_address;
    ip4_addr_t ip4_netmask;
    struct netif *pnetif;

    pnetif = netif_find((char *)ifname);
    if (pnetif == RT_NULL) {
        rt_kprintf("Can find network interface! \r\n");
        return -1;
    }
    if (ip4addr_aton(ip_addr, &ip4_address) == 0 || ip4addr_aton(nm_addr, &ip4_netmask) == 0) {
        return -1;
    }
    route_ip4_add(&ip4_address, &ip4_netmask, pnetif);
    return 0;
}

/*********************************************************
**   function name:         route4_delete
**   Descriptions:          删除IPv4路由表
**   input parameters:      nm_addr:删除的子网掩码
**                          ip_addr:删除的IP地址
**   outout parameters:     无
**   Returned value:        无
*********************************************************/
int route4_delete(char* ip_addr, char* nm_addr)
{
    ip4_addr_t ip4_address;
    ip4_addr_t ip4_netmask;

    if (ip4addr_aton(ip_addr, &ip4_address) == 0 || ip4addr_aton(nm_addr, &ip4_netmask) == 0) {
        return -1;
    }

    route_ip4_delete(&ip4_address, &ip4_netmask);
    return 0;
}
#endif

#if LWIP_IPV6
/*********************************************************
**   function name:         route6_add
**   Descriptions:          设置一个naptpt接口
**   input parameters:      ifname: 指定网络接口
**                          ip6_addr:添加的IPv6地址前缀
**   outout parameters:     无
**   Returned value:        无
*********************************************************/
int route6_add(char* ifname, char* ip6_addr)
{
    ip6_addr_t ip6_address;
    struct netif *pnetif;

    pnetif = netif_find((char *)ifname);
    if (pnetif == RT_NULL) {
        rt_kprintf("Can find network interface! \r\n");
        return -1;
    }

    if (ip6addr_aton(ip6_addr, (ip6_addr_t *)&ip6_address) == 0) {
        return -1;
    }

    route_ip6_add(&ip6_address, pnetif);
    return 0;
}

/*********************************************************
**   function name:         route6_delete
**   Descriptions:          删除IPv6路由表
**   input parameters:
**                          ip_addr:删除的路由表IP地址前缀
**   outout parameters:     无
**   Returned value:        无
*********************************************************/
int route6_delete(char* ip6_addr)
{
    ip6_addr_t ip6_address;

    if (ip6addr_aton(ip6_addr, (ip6_addr_t *)&ip6_address) == 0) {
        return -1;
    }

    route_ip6_delete(&ip6_address);
    return 0;
}
#endif

void usage()
{
    rt_kprintf("IPv4 Command: route inet add/del -net/-host TARGET netmask dev/device \n");
    rt_kprintf("IPv6 Command: route inet6 add/del -net TARGET dev/device\n");
    rt_kprintf("Command: route -n find route  route -help is help   \n");
    return ;
}

#if LWIP_IPV6
/* IPv6 add/del route item in route table */
/* main part of this function is from net-tools inet6_sr.c file */
static int inet6_setroute(int action, char **args)
{
    char target[64] = {0};
    char devname[8] = {0};         /* device name */

    args++;
    if (rt_strlen(*args) < 64) {
        strcpy(target, *args);
    } else {
        usage();
        return -1;
    }

    args++;
    if (!strcmp(*args, "device") || !strcmp(*args, "dev"))
    {
        args++;
        if (rt_strlen(*args) < 8) {
            strcpy(devname, *args);
        }
    } else {
        usage();
        return -1;
    }

    /* Tell the kernel to accept this route. */
    if (action == RTACTION_ADD)
    {
        if (route6_add(devname, target) != 0)
        {
            rt_kprintf("route6_add  failure \n");
            return -1;
        }
    } else {
        if (route6_delete(target) != 0)
        {
            rt_kprintf("route6_delete  failure \n");
            return -1;
        }
    }
    return (0);
}
#endif
/*
 *  IPv4 add/del route item in route table
 */
int inet_setroute(int action, char **args)
{
    char target[16] = {0};
    char netmask[16] = {0};
    char ifname[8] = {0};

    while(args)
    {
        if (*args == NULL)
        {
            break;
        }
        if (!strcmp(*args, "-net") || !strcmp(*args, "-host"))
        {/* default is a network target */
            args++;
            if (rt_strlen(*args) < 16) {
                strcpy(target, *args);
            }
            args++;
            continue;
        }
        if (!strcmp(*args, "netmask"))
        {/* netmask setting */
            args++;
            if (rt_strlen(*args) < 16) {
                strcpy(netmask, *args);
            }
            args++;
            continue;
        }
        if (!strcmp(*args, "device") || !strcmp(*args, "dev"))
        {/* device setting */
            args++;
            if (rt_strlen(*args) < 8) {
                strcpy(ifname, *args);
            }
            args++;
            continue;
        }
        /* if you have other options, please put them in this place,
          like the options above. */
    }

    /* tell the kernel to accept this route */
    if (action == RTACTION_DEL)
    {/* del a route item */
        if (route4_delete(target, netmask) != 0)
        {
            rt_kprintf("route4_delete  failure \n");
            return -1;
        }
    } else {/* add a route item */
        if (route4_add(ifname, target, netmask) != 0)
        {
            rt_kprintf("route4_add  failure \n");
            return -1;
        }
    }
    return 0;
}

#ifdef RT_USING_FINSH
#include <finsh.h>

FINSH_FUNCTION_EXPORT(route, route network host);
/*
 * If no argument is specified, display the route information;
 * If there are 3 arguments, mount the filesystem.
 * The order of the arguments is:
 * argv[1]: device name
 * argv[2]: mountpoint path
 * argv[3]: filesystem type
 */
static int cmd_route(int argc, char **argv)
{
    int action = 0;

    if (argc == 2) {
        if (!strcmp(argv[1], "-n")) {
            inet_findroute();
        } else {
            usage();
        }
        return 0;
    }
    if (argc < 7) {
        usage();
        return 0;
    }

    if (!strcmp(argv[2], "add"))
    {
        action = RTACTION_ADD;
    } else if (!strcmp(argv[2], "del")) {
        action = RTACTION_DEL;
    } else {
        usage();
        return 0;
    }

    /* add or del a ipv4 route item */
    if (!strcmp(argv[1], "inet") && argc == 9)
    {
        inet_setroute(action, argv+3);
    }
    /* add  or del a ipv6 route item */
#if LWIP_IPV6
    if (!strcmp(argv[1], "inet6") && argc == 7)
    {
        inet6_setroute(action, argv+3);
    }
#endif
    return 0;
}
MSH_CMD_EXPORT_ALIAS(cmd_route, route, route <add/del> <-host/-net> <device>);
#endif

#endif
