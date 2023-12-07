/**
 * @file
 * An implementation of the TCP/IP protocol suite
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

#include "lwip/opt.h"

#include "lwip/priv/sockets_priv.h"

#ifdef LWIP_TIMESTAMPS
/** Submit a error pbuf to sock
 */
int sock_queue_err_pbuf(struct lwip_sock *sk, struct pbuf *p)
{
  int qidx;

  if (!sk || !p) {
    return -1;
  }

  sys_mutex_lock(&sk->p_error_qlock);

  qidx = sk->p_error_qidx++;
  if (sk->p_error_queue[qidx]) {
    /* If Write Back Release */
    pbuf_free(sk->p_error_queue[qidx]);
  }
  sk->p_error_queue[qidx] = p;

  sk->p_error_qidx %= LWIP_ARRAYSIZE(sk->p_error_queue);

  sys_mutex_unlock(&sk->p_error_qlock);

  return 0;
}

/** Extract a error pbuf to sock
 */
struct pbuf *sock_dequeue_err_pbuf(struct lwip_sock *sk)
{
  struct pbuf *p = NULL;

  if (!sk) {
    goto _ret;
  }

  if (sk->p_error_last_qidx != sk->p_error_qidx) {
    int qidx;

    sys_mutex_lock(&sk->p_error_qlock);

    qidx = sk->p_error_last_qidx;
    /* current is NULL is IMPOSSIBLE */
    p = sk->p_error_queue[qidx];
    sk->p_error_queue[qidx] = NULL;

    sk->p_error_last_qidx++;
    sk->p_error_qidx %= LWIP_ARRAYSIZE(sk->p_error_queue);

    sys_mutex_unlock(&sk->p_error_qlock);
  }

_ret:
  return p;
}
#endif /* LWIP_TIMESTAMPS */
