/**
 * @file
 * lwip route list module
 * This is a sample route list implementation, which is supported to ipv4 and ipv6.
 */

/*
 * Copyright (c) 2001-2004 Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Ren.Haibo <habbyren@qq.com>
 *
 */

/* [LWIPROUTE_ROUTE_0515] */
/* ip_route.c
 *
 * The code for routing the ip packet, support to ipv4 and ipv6.
 *
 */

#include "lwip/opt.h"

#if LWIP_ROUTE /* don't build if not configured for use in lwipopts.h */

#include "lwip/ip_route.h"
#include "lwip/udp.h"
#include "lwip/def.h"
#include "lwip/memp.h"
#include "lwip/inet_chksum.h"
#include "lwip/ip_addr.h"
#include "lwip/ip6.h"
#include "lwip/ip6_addr.h"
#include "lwip/inet_chksum.h"
#include "lwip/netif.h"
#include "lwip/icmp.h"
#include "lwip/icmp6.h"
#include "lwip/stats.h"
#include "lwip/snmp.h"
#include "arch/perf.h"
#include "lwip/dhcp.h"

#include <string.h>

/** Generic route list structure used for lwip. */
struct route_node {
  /** pointer to next in linked list */
  struct route_node *next;
  struct route_node *prev;
  
#if LWIP_IPV4   
  /** The routing IP address and netmask for this netif. This is in network byte order */
  ip4_addr_t ip_addr;
  ip4_addr_t netmask;
#endif
#if LWIP_IPV6
   /** The routing IPv6 address for this netif. */
  ip6_addr_t ip6_addr;
#endif
  /** The IP packet is routing to which packet. */
  struct netif *netif;
};

/* The list of lwip route */
struct route_node *route_list;

#if LWIP_IPV4
/**
 * Add a route for ipv4.
 *
 * @param ip_addr The IP address which is for routing.
 * @param netmask The netmask which is for routing.
 * @param inp network interface on which the datagram was received.
 * @return ERR_OK Add the route OK,
 *         Others Add the route failed.
 */
err_t
route_ip4_add(ip4_addr_t *ip_addr, ip4_addr_t *netmask, struct netif *inp)
{
    struct route_node *rt_node;

    /** If the ipaddr or netif is illegal, just return. */
    if((ip_addr == NULL) || (inp ==NULL)) {
        return ERR_VAL;
    }

    if (route_ip4_find(ip_addr))
    {
        return ERR_VAL;
    }

    rt_node = (struct route_node *)mem_malloc(sizeof(struct route_node));
    
    LWIP_ERROR("rt_node != NULL", (rt_node != NULL), return ERR_MEM;);
    
    memset(rt_node, 0, sizeof(struct route_node));
    
    rt_node->ip_addr = *ip_addr;
    rt_node->netmask = *netmask;
    rt_node->netif   =inp;
    
    /** Add the route node to the list header. */
    rt_node->next = route_list;
    rt_node->prev = NULL;
    if(route_list) {
        route_list->prev = rt_node;
    }
    route_list = rt_node;

#if defined(SAL_USING_AF_NETLINK)
#if LWIP_IPV4
    rtnl_route_notify(ip_addr->addr, netmask->addr, inp->ip_addr.addr, RTM_NEWROUTE);
#endif
#endif

    return ERR_OK;
}

/**
 * Delete a route for ipv4.
 *
 * @param ip_addr The IP address which is in the route list, needed to be deleted.
 * @param netmask The netmask which is in the route list, needed to be deleted.
 * @return ERR_OK Delete the route OK,
 *         Others Delete the route failed.
 */
err_t
route_ip4_delete(ip4_addr_t *ip_addr, ip4_addr_t *netmask)
{
    struct route_node *rt_node;

    /** If the ipaddr is illegal, just return. */
    if((ip_addr == NULL) || (netmask ==NULL)) {
        return ERR_VAL;
    }
    
    for(rt_node=route_list; rt_node!=NULL; rt_node=rt_node->next) {
        if(((rt_node->ip_addr.addr) == (ip_addr->addr)) && 
           ((rt_node->netmask.addr) == (netmask->addr))) {
            if (rt_node->prev == NULL) {
                route_list = rt_node->next;
            } else {
                rt_node->prev->next = rt_node->next;
            }
            if (rt_node->next) {
                rt_node->next->prev = rt_node->prev;
            }
            mem_free(rt_node);
            return ERR_OK;
        }
    }

#if defined(SAL_USING_AF_NETLINK)
    rtnl_route_notify(ip_addr->addr, netmask->addr, 0, RTM_DELROUTE);
#endif

    return ERR_OK;
}

/**
 * Find a route for ipv4.
 *
 * @param ip_addr The IPv4 address which is for routing.
 * @return NULL No route found,
 *         netif  The packet need to route to this network inferface.
 */
struct netif *
route_ip4_find(ip4_addr_t *ip_addr)
{
    struct route_node *rt_node;
    
    /** If the ipaddr is illegal, just return. */
    if(ip_addr == NULL) {
        return NULL;
    }
    
    for(rt_node=route_list; rt_node!=NULL; rt_node=rt_node->next) {
        /** If this is not a ipv4 route list. */
        if(rt_node->netmask.addr == 0) {
           continue;
        }
        
        /** If it is a boardcast address **/
        if((ip_addr->addr == 0xffffffff) && (rt_node->ip_addr.addr == ip_addr->addr)) {
            return rt_node->netif;
        }
        
        if((rt_node->ip_addr.addr & rt_node->netmask.addr) == (ip_addr->addr & rt_node->netmask.addr)){
            return rt_node->netif;
        }
    }
    
    return NULL;
}

err_t
inet_findroute()
{
    struct route_node *rt_node;

    rt_kprintf("Destination     Gateway         Genmask         Iface\n");

    for(rt_node=route_list; rt_node!=NULL; rt_node=rt_node->next) {
        /** If this is not a ipv4 route list. */
        if(rt_node->netmask.addr == 0) {
#if LWIP_IPV6
            rt_kprintf("ip6_addr:%s\n", ip6addr_ntoa(&rt_node->ip6_addr));
#endif
            continue;
        } else {
            rt_kprintf("%-16s", ip4addr_ntoa(&rt_node->ip_addr));
            rt_kprintf("%-16s", "*");
            rt_kprintf("%-16s", ip4addr_ntoa(&rt_node->netmask));
            rt_kprintf("%s\n", rt_node->netif->name);
        }
    }
    return ERR_OK;
}

int inet_route_foreach(void (*func)(const char *name, uint32_t ip_addr, uint32_t netmask, void *parameter), void *parameter)
{
    struct route_node *rt_node;

    for (rt_node = route_list; rt_node != NULL; rt_node = rt_node->next)
    {
        /** If this is not a ipv4 route list. */
        if (rt_node->netmask.addr == 0)
        {
#if LWIP_IPV6
            //todo
#endif
        }
        else if (func)
        {
            func(rt_node->netif->name, rt_node->ip_addr.addr, rt_node->netmask.addr, parameter);
        }
    }
    return 0;
}

#if defined(SAL_USING_AF_NETLINK)
int
route_for_each_notify(struct msg_buf *msg, int (*cb)(struct msg_buf *m_buf, uint32_t addr, uint32_t netmask, uint32_t sre_addr, int rt_num, int index))
{
    struct route_node *rt_node;
    int rt_num = 0;
    int index = 0;

    rt_node=route_list;
    while(rt_node != NULL)
    {
        rt_num ++;
        rt_node=rt_node->next;
    }

    for(rt_node=route_list; rt_node!=NULL; rt_node=rt_node->next) {
        // if (cb(msg, rt_node->ip_addr.addr, rt_node->netmask.addr, rt_node->netif->ip_addr, rt_num, index) < 0)
        if (cb(msg, rt_node->ip_addr.addr, rt_node->netmask.addr, 0, rt_num, index) < 0)
            return ERR_MEM;

        index ++;
    }

    return ERR_OK;
}
#endif

#endif

#if LWIP_IPV6
/**
 * Add a route for ipv6.
 *
 * @param ip6_addr The IPv6 address which is for routing.
 * @param inp network interface on which the datagram was received.
 *
 */
err_t
route_ip6_add(ip6_addr_t *ip6_addr, struct netif *inp)
{
    struct route_node *rt_node;
    /** If the ipaddr or netif is illegal, just return. */
    if((ip6_addr == NULL) || (inp ==NULL)) {
        return ERR_VAL;
    }
    
    rt_node = (struct route_node *)mem_malloc(sizeof(struct route_node));
    
    LWIP_ERROR("rt_node != NULL", (rt_node != NULL), return ERR_MEM;);
    
    memset(rt_node, 0, sizeof(struct route_node));
    
    rt_node->ip6_addr = *ip6_addr;
    rt_node->netif   =inp;
    
    /** Add the route node to the list header. */
    rt_node->next = route_list;
    rt_node->prev = NULL;
    if(route_list) {
        route_list->prev = rt_node;
    }
    route_list = rt_node;
    
    return ERR_OK;
}

/**
 * Delete a route for ipv6.
 *
 * @param ip6_addr The IPv6 address which is in the route list, needed to be deleted.
 *
 */
err_t
route_ip6_delete(ip6_addr_t *ip6_addr)
{
    struct route_node *rt_node;

    /** If the ipaddr is illegal, just return. */
    if(ip6_addr == NULL) {
        return ERR_VAL;
    }
    
    for(rt_node=route_list; rt_node!=NULL; rt_node=rt_node->next) {
        if((rt_node->ip6_addr.addr[0] == ip6_addr->addr[0]) &&
           (rt_node->ip6_addr.addr[1] == ip6_addr->addr[1]) &&
           (rt_node->ip6_addr.addr[2] == ip6_addr->addr[2]) &&
           (rt_node->ip6_addr.addr[3] == ip6_addr->addr[3])) {
            if (rt_node->prev == NULL) {
                route_list = rt_node->next;
            } else {
                rt_node->prev->next = rt_node->next;
            }
            if (rt_node->next) {
                rt_node->next->prev = rt_node->prev;
            }
            mem_free(rt_node);
            return ERR_OK;
        }
    }
    
    return ERR_OK;
}

/**
 * Find a route for ipv6.
 *
 * @param ip6_addr The IPv6 address which is for routing.
 * @param inp network interface on which the datagram was received.
 * @return NULL No route found,
 *         netif  The packet need to route to this network inferface.
 */
struct netif *
route_ip6_find(ip6_addr_t *ip6_addr)
{
    struct route_node *rt_node;

    /** If the ipaddr is illegal, just return. */
    if(ip6_addr == NULL) {
        return NULL;
    }
    
    for(rt_node=route_list; rt_node!=NULL; rt_node=rt_node->next) {
        if((rt_node->ip6_addr.addr[0] == ip6_addr->addr[0]) &&
           (rt_node->ip6_addr.addr[1] == ip6_addr->addr[1])) {
            return rt_node->netif;
        }
    }
    
    return NULL;
}
#endif

#endif /* LWIP_ROUTE */
