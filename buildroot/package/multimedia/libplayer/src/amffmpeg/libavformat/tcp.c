/*
 * TCP protocol
 * Copyright (c) 2002 Fabrice Bellard
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */
#include "avformat.h"
#include "libavutil/parseutils.h"
#include <unistd.h>
#include <errno.h>
#include "internal.h"
#include "network.h"
#include "os_support.h"
#include "url.h"
#if HAVE_POLL_H
#include <poll.h>
#endif
#include <sys/time.h>
#define closesocket close
typedef struct TCPContext {
    int fd;
} TCPContext;

/* return non zero if error */
static int tcp_open(URLContext *h, const char *uri, int flags)
{
    struct addrinfo hints, *ai, *cur_ai;
    int port, fd = -1;
    TCPContext *s = NULL;
    int listen_socket = 0;
    const char *p;
    char buf[256];
    int ret;
    socklen_t optlen;
    int timeout = 500, listen_timeout = -1;
    char hostname[1024],proto[1024],path[1024];
    char portstr[10];

    av_url_split(proto, sizeof(proto), NULL, 0, hostname, sizeof(hostname),
        &port, path, sizeof(path), uri);
    if (strcmp(proto, "tcp"))
        return AVERROR(EINVAL);
    if (port <= 0 || port >= 65536) {
        av_log(h, AV_LOG_ERROR, "Port missing in uri\n");
        return AVERROR(EINVAL);
    }
    p = strchr(uri, '?');
    if (p) {
        if (av_find_info_tag(buf, sizeof(buf), "listen", p))
            listen_socket = 1;
        if (av_find_info_tag(buf, sizeof(buf), "timeout", p)) {
            timeout = strtol(buf, NULL, 10);
        }
        if (av_find_info_tag(buf, sizeof(buf), "listen_timeout", p)) {
            listen_timeout = strtol(buf, NULL, 10);

        }
    }
    memset(&hints, 0, sizeof(hints));
    if(am_getconfig_bool_def("media.libplayer.ipv4only",1))	
    		hints.ai_family = AF_INET;
    else
		hints.ai_family = AF_UNSPEC;	
    hints.ai_socktype = SOCK_STREAM;
    snprintf(portstr, sizeof(portstr), "%d", port);
	av_log(h, AV_LOG_INFO,"tcp will get address from dns!\n");	
    if (listen_socket)
        hints.ai_flags |= AI_PASSIVE;
    if (!hostname[0])
        ret = getaddrinfo(NULL, portstr, &hints, &ai);
    else
    ret = getaddrinfo(hostname, portstr, &hints, &ai);
    if (ret) {
        av_log(h, AV_LOG_ERROR,
               "Failed to resolve hostname %s: %s\n",
               hostname, gai_strerror(ret));
        return AVERROR(EIO);
    }
    av_log(h, AV_LOG_INFO,"resolved %s's  ipaddress \n",hostname);
    cur_ai = ai;

 restart:
    ret = AVERROR(EIO);
    fd = socket(cur_ai->ai_family, cur_ai->ai_socktype, cur_ai->ai_protocol);
    if (fd < 0)
        goto fail;

    if (listen_socket) {
        int fd1;
        int reuse = 1;
        struct pollfd lp = { fd, POLLIN, 0 };
        setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
        ret = bind(fd, cur_ai->ai_addr, cur_ai->ai_addrlen);
        if (ret) {
            ret = ff_neterrno();
            goto fail1;
        }
        ret = listen(fd, 1);
        if (ret) {
            ret = ff_neterrno();
            goto fail1;
        }
        ret = poll(&lp, 1, listen_timeout >= 0 ? listen_timeout : -1);
        if (ret <= 0) {
            ret = AVERROR(ETIMEDOUT);
            goto fail1;
        }
        fd1 = accept(fd, NULL, NULL);
        if (fd1 < 0) {
            ret = ff_neterrno();
            goto fail1;
        }
        closesocket(fd);
        fd = fd1;
        ff_socket_nonblock(fd, 1);
    } else {
 redo:
        ff_socket_nonblock(fd, 1);
        ret = connect(fd, cur_ai->ai_addr, cur_ai->ai_addrlen);
    }

    if (ret < 0) {
        struct pollfd p = {fd, POLLOUT, 0};
        ret = ff_neterrno();
        if (ret == AVERROR(EINTR)) {
            if (url_interrupt_cb()) {
                ret = AVERROR_EXIT;
                goto fail1;
            }
            goto redo;
        }
        if (ret != AVERROR(EINPROGRESS) &&
            ret != AVERROR(EAGAIN))
            goto fail;

        /* wait until we are connected or until abort */
        while(timeout--) {
            if (url_interrupt_cb()) {
                ret = AVERROR_EXIT;
                goto fail1;
            }
            ret = poll(&p, 1, 100);
            if (ret > 0)
                break;
        }
        if (ret <= 0) {
			 av_log(h, AV_LOG_ERROR,
                   "TCP connection to %s:%d timeout failed!\n",
                   hostname, port);
            ret = AVERROR(ETIMEDOUT);
            goto fail;
        }
        /* test error */
        optlen = sizeof(ret);
        if (getsockopt (fd, SOL_SOCKET, SO_ERROR, &ret, &optlen))
            ret = AVUNERROR(ff_neterrno());
        if (ret != 0) {
            char errbuf[100];
            ret = AVERROR(ret);
            av_strerror(ret, errbuf, sizeof(errbuf));
            av_log(h, AV_LOG_ERROR,
                   "TCP connection to %s:%d failed: %s, ret = %d\n",
                   hostname, port, errbuf, ret);            
            ret = AVERROR(ret);
            if(ret>0)
                ret = AVERROR(EIO);
            goto fail;
        }
    }
    av_log(h, AV_LOG_INFO,"tcp  connect %s ok!\n",hostname);	
    s = av_malloc(sizeof(TCPContext));
    if (!s) {
        freeaddrinfo(ai);
        return AVERROR(ENOMEM);
    }
    h->priv_data = s;
    h->is_streamed = 1;
    s->fd = fd;
    freeaddrinfo(ai);
    return 0;

 fail:
    if (cur_ai->ai_next) {
        /* Retry with the next sockaddr */
        cur_ai = cur_ai->ai_next;
        if (fd >= 0)
            closesocket(fd);
        goto restart;
    }
 fail1:
    if (fd >= 0)
        closesocket(fd);
    freeaddrinfo(ai);
    return ret;
}

static int tcp_read(URLContext *h, uint8_t *buf, int size)
{
    TCPContext *s = h->priv_data;
    int ret;

    if (!(h->flags & AVIO_FLAG_NONBLOCK)) {
        ret = ff_network_wait_fd(s->fd, 0);
        if (ret < 0){
            av_log(h, AV_LOG_INFO,"ff_network_wait_fd return error %d,errmsg:%s \n",ret,strerror(errno)!=NULL?strerror(errno):"unkown");
            return ret;

        }
    }
    ret = recv(s->fd, buf, size, 0);
    if(ret<=0){
        av_log(h, AV_LOG_INFO,"tcp_read return error %d,errno:%d,errmsg:%s \n",ret,errno,strerror(errno)!=NULL?strerror(errno):"unkown");
    }
    return ret < 0 ? ff_neterrno() : ret;
}

static int tcp_write(URLContext *h, const uint8_t *buf, int size)
{
    TCPContext *s = h->priv_data;
    int ret;

    if (!(h->flags & AVIO_FLAG_NONBLOCK)) {
        ret = ff_network_wait_fd(s->fd, 1);
        if (ret < 0)
            return ret;
    }
    ret = send(s->fd, buf, size, 0);
    return ret < 0 ? ff_neterrno() : ret;
}

static int tcp_close(URLContext *h)
{
    TCPContext *s = h->priv_data;
    closesocket(s->fd);
    av_free(s);
    return 0;
}

static int tcp_get_file_handle(URLContext *h)
{
    TCPContext *s = h->priv_data;
    return s->fd;
}

URLProtocol ff_tcp_protocol = {
    .name                = "tcp",
    .url_open            = tcp_open,
    .url_read            = tcp_read,
    .url_write           = tcp_write,
    .url_close           = tcp_close,
    .url_get_file_handle = tcp_get_file_handle,
};
