/**
 * @file
 * Userspace API for hardware time stamping of network packets
 */

/*
 * Copyright (c) 2006-2023, RT-Thread Development Team.
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
 * Author: GuEe-GUI <GuEe-GUI@github.com>
 *
 */

#ifndef LWIP_NET_TSTAMP_H
#define LWIP_NET_TSTAMP_H

#include "lwip/arch.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * SO_TIMESTAMP** for setsockopt.
 */
#define SO_SELECT_ERR_QUEUE   45

#define SO_TIMESTAMP_OLD      29
#define SO_TIMESTAMPNS_OLD    35
#define SO_TIMESTAMPING_OLD   37

#define SO_TIMESTAMP_NEW      63
#define SO_TIMESTAMPNS_NEW    64
#define SO_TIMESTAMPING_NEW   65

#ifdef LWIP_HAVE_INT64
#define SO_TIMESTAMP          SO_TIMESTAMP_OLD
#define SO_TIMESTAMPNS        SO_TIMESTAMPNS_OLD
#define SO_TIMESTAMPING       SO_TIMESTAMPING_OLD
#else
#define SO_TIMESTAMP          (sizeof(time_t) == sizeof(long) ? SO_TIMESTAMP_OLD : SO_TIMESTAMP_NEW)
#define SO_TIMESTAMPNS        (sizeof(time_t) == sizeof(long) ? SO_TIMESTAMPNS_OLD : SO_TIMESTAMPNS_NEW)
#define SO_TIMESTAMPING       (sizeof(time_t) == sizeof(long) ? SO_TIMESTAMPING_OLD : SO_TIMESTAMPING_NEW)
#endif /* LWIP_HAVE_INT64 */

/**
 * @ingroup net_tstamp
 * SO_TIMESTAMPING flags */
enum {
  SOF_TIMESTAMPING_TX_HARDWARE  = (1 << 0),
  SOF_TIMESTAMPING_TX_SOFTWARE  = (1 << 1),
  SOF_TIMESTAMPING_RX_HARDWARE  = (1 << 2),
  SOF_TIMESTAMPING_RX_SOFTWARE  = (1 << 3),
  SOF_TIMESTAMPING_SOFTWARE     = (1 << 4),
  SOF_TIMESTAMPING_SYS_HARDWARE = (1 << 5),
  SOF_TIMESTAMPING_RAW_HARDWARE = (1 << 6),
  SOF_TIMESTAMPING_OPT_ID       = (1 << 7),
  SOF_TIMESTAMPING_TX_SCHED     = (1 << 8),
  SOF_TIMESTAMPING_TX_ACK       = (1 << 9),
  SOF_TIMESTAMPING_OPT_CMSG     = (1 << 10),
  SOF_TIMESTAMPING_OPT_TSONLY   = (1 << 11),
  SOF_TIMESTAMPING_OPT_STATS    = (1 << 12),
  SOF_TIMESTAMPING_OPT_PKTINFO  = (1 << 13),
  SOF_TIMESTAMPING_OPT_TX_SWHW  = (1 << 14),
  SOF_TIMESTAMPING_BIND_PHC     = (1 << 15),
  SOF_TIMESTAMPING_OPT_ID_TCP   = (1 << 16),
  SOF_TIMESTAMPING_LAST         = SOF_TIMESTAMPING_OPT_ID_TCP,
  SOF_TIMESTAMPING_MASK         = (SOF_TIMESTAMPING_LAST - 1) | SOF_TIMESTAMPING_LAST,
};

/*
 * SO_TIMESTAMPING flags are either for recording a packet timestamp or for
 * reporting the timestamp to user space.
 * Recording flags can be set both via socket options and control messages.
 */
#define SOF_TIMESTAMPING_TX_RECORD_MASK (SOF_TIMESTAMPING_TX_HARDWARE | \
    SOF_TIMESTAMPING_TX_SOFTWARE | \
    SOF_TIMESTAMPING_TX_SCHED | \
    SOF_TIMESTAMPING_TX_ACK)

/** struct so_timestamping - SO_TIMESTAMPING parameter */
struct so_timestamping {
  /** SO_TIMESTAMPING flags */
  int flags;
  /** Index of PTP virtual clock bound to sock. This is available if flag SOF_TIMESTAMPING_BIND_PHC is set. */
  int bind_phc;
};

/** struct hwtstamp_config - %SIOCGHWTSTAMP and %SIOCSHWTSTAMP parameter.
 * %SIOCGHWTSTAMP and %SIOCSHWTSTAMP expect a &struct ifreq with a
 * ifr_data pointer to this structure.  For %SIOCSHWTSTAMP, if the
 * driver or hardware does not support the requested @rx_filter value,
 * the driver may use a more general filter mode.  In this case
 * @rx_filter will indicate the actual mode on return.
 */
struct hwtstamp_config {
  /* one of HWTSTAMP_FLAG_* */
  int flags;
  /* one of HWTSTAMP_TX_* */
  int tx_type;
  /* one of HWTSTAMP_FILTER_* */
  int rx_filter;
};

/**
 * @ingroup net_tstamp
 * possible values for hwtstamp_config->flags */
enum hwtstamp_flags {
  /*
   * With this flag, the user could get bond active interface's
   * PHC index. Note this PHC index is not stable as when there
   * is a failover, the bond active interface will be changed, so
   * will be the PHC index.
   */
  HWTSTAMP_FLAG_BONDED_PHC_INDEX = (1 << 0),
#define HWTSTAMP_FLAG_BONDED_PHC_INDEX  HWTSTAMP_FLAG_BONDED_PHC_INDEX

  HWTSTAMP_FLAG_LAST = HWTSTAMP_FLAG_BONDED_PHC_INDEX,
  HWTSTAMP_FLAG_MASK = (HWTSTAMP_FLAG_LAST - 1) | HWTSTAMP_FLAG_LAST
};

/**
 * @ingroup net_tstamp
 * possible values for hwtstamp_config->tx_type */
enum hwtstamp_tx_types {
  /*
   * No outgoing packet will need hardware time stamping;
   * should a packet arrive which asks for it, no hardware
   * time stamping will be done.
   */
  HWTSTAMP_TX_OFF,

  /*
   * Enables hardware time stamping for outgoing packets;
   * the sender of the packet decides which are to be
   * time stamped by setting %SOF_TIMESTAMPING_TX_SOFTWARE
   * before sending the packet.
   */
  HWTSTAMP_TX_ON,

  /*
   * Enables time stamping for outgoing packets just as
   * HWTSTAMP_TX_ON does, but also enables time stamp insertion
   * directly into Sync packets. In this case, transmitted Sync
   * packets will not received a time stamp via the socket error
   * queue.
   */
  HWTSTAMP_TX_ONESTEP_SYNC,

  /*
   * Same as HWTSTAMP_TX_ONESTEP_SYNC, but also enables time
   * stamp insertion directly into PDelay_Resp packets. In this
   * case, neither transmitted Sync nor PDelay_Resp packets will
   * receive a time stamp via the socket error queue.
   */
  HWTSTAMP_TX_ONESTEP_P2P,

  /* add new constants above here */
  __HWTSTAMP_TX_CNT
};

/**
 * @ingroup net_tstamp
 * possible values for hwtstamp_config->rx_filter */
enum hwtstamp_rx_filters {
  /* time stamp no incoming packet at all */
  HWTSTAMP_FILTER_NONE,

  /* time stamp any incoming packet */
  HWTSTAMP_FILTER_ALL,

  /* return value: time stamp all packets requested plus some others */
  HWTSTAMP_FILTER_SOME,

  /* PTP v1, UDP, any kind of event packet */
  HWTSTAMP_FILTER_PTP_V1_L4_EVENT,
  /* PTP v1, UDP, Sync packet */
  HWTSTAMP_FILTER_PTP_V1_L4_SYNC,
  /* PTP v1, UDP, Delay_req packet */
  HWTSTAMP_FILTER_PTP_V1_L4_DELAY_REQ,
  /* PTP v2, UDP, any kind of event packet */
  HWTSTAMP_FILTER_PTP_V2_L4_EVENT,
  /* PTP v2, UDP, Sync packet */
  HWTSTAMP_FILTER_PTP_V2_L4_SYNC,
  /* PTP v2, UDP, Delay_req packet */
  HWTSTAMP_FILTER_PTP_V2_L4_DELAY_REQ,

  /* 802.AS1, Ethernet, any kind of event packet */
  HWTSTAMP_FILTER_PTP_V2_L2_EVENT,
  /* 802.AS1, Ethernet, Sync packet */
  HWTSTAMP_FILTER_PTP_V2_L2_SYNC,
  /* 802.AS1, Ethernet, Delay_req packet */
  HWTSTAMP_FILTER_PTP_V2_L2_DELAY_REQ,

  /* PTP v2/802.AS1, any layer, any kind of event packet */
  HWTSTAMP_FILTER_PTP_V2_EVENT,
  /* PTP v2/802.AS1, any layer, Sync packet */
  HWTSTAMP_FILTER_PTP_V2_SYNC,
  /* PTP v2/802.AS1, any layer, Delay_req packet */
  HWTSTAMP_FILTER_PTP_V2_DELAY_REQ,

  /* NTP, UDP, all versions and packet modes */
  HWTSTAMP_FILTER_NTP_ALL,

  /* add new constants above here */
  __HWTSTAMP_FILTER_CNT
};

/** SCM_TIMESTAMPING_PKTINFO control message */
struct scm_ts_pktinfo {
  u32_t if_index;
  u32_t pkt_length;
  u32_t reserved[2];
};

/**
 * @ingroup net_tstamp
 * SO_TXTIME gets a struct sock_txtime with flags being an integer bit
 * field comprised of these values. */
enum txtime_flags {
  SOF_TXTIME_DEADLINE_MODE = (1 << 0),
  SOF_TXTIME_REPORT_ERRORS = (1 << 1),

  SOF_TXTIME_FLAGS_LAST = SOF_TXTIME_REPORT_ERRORS,
  SOF_TXTIME_FLAGS_MASK = (SOF_TXTIME_FLAGS_LAST - 1) | SOF_TXTIME_FLAGS_LAST
};

/** SO_TXTIME value */
struct sock_txtime {
  /* reference clockid */
  int clockid;
  /* as defined by enum txtime_flags */
  u32_t flags;
};

#ifdef __cplusplus
}
#endif

#endif /* LWIP_NET_TSTAMP_H */
