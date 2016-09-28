/*
 * libcurl registered as ffmpeg protocol
 * Copyright (c) amlogic,2013, senbai.tao<senbai.tao@amlogic.com>
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


#include "libavformat/avformat.h"
#include "libavformat/internal.h"
#include "libavformat/url.h"
#include "libavutil/avstring.h"
#include "libavutil/opt.h"
#include "curl_fetch.h"
#include "curl_log.h"

typedef struct _CURLFFContext {
    char uri[MAX_CURL_URI_SIZE];
    CFContext * cfc_h;
    //pthread_t monitor_pid;
    //int monitor_quited;
} CURLFFContext;

static const AVOption options[] = {{NULL}};
static const AVClass curlffmpeg_class = {
    .class_name     = "Amlcurlffmpeg",
    .item_name      = av_default_item_name,
    .option             = options,
    .version            = LIBAVUTIL_VERSION_INT,
};

//static void * curl_ffmpeg_interrupt_monitor_thread(void *_handle);

static void curl_ffmpeg_register_interrupt(CURLFFContext *h, interruptcallback pfunc)
{
    if (!h || !h->cfc_h) {
        return;
    }
    curl_fetch_register_interrupt(h->cfc_h, pfunc);
    return;
}

static int curl_ffmpeg_open(URLContext *h, const char *uri, int flags)
{
    CLOGI("curl_ffmpeg_open enter, flags=%d\n", flags);
    int ret = -1;
    CURLFFContext * handle = NULL;
    if (!uri || strlen(uri) < 1 || strlen(uri) > MAX_CURL_URI_SIZE) {
        CLOGE("Invalid curl-ffmpeg uri\n");
        return ret;
    }
    handle = (CURLFFContext *)av_mallocz(sizeof(CURLFFContext));
    if (!handle) {
        CLOGE("Failed to allocate memory for CURLFFContext handle\n");
        return ret;
    }
    memset(handle->uri, 0, sizeof(handle->uri));
    if (av_stristart(uri, "curl:", NULL)) {
        av_strlcpy(handle->uri, uri + 5, sizeof(handle->uri));
    } else {
        av_strlcpy(handle->uri, uri, sizeof(handle->uri));
    }
    handle->cfc_h = curl_fetch_init(handle->uri, h->headers, flags);
    if (!handle->cfc_h) {
        CLOGE("curl_fetch_init failed\n");
        return ret;
    }
    curl_ffmpeg_register_interrupt(handle, url_interrupt_cb);
    /*
    handle->monitor_quited = 0;
    ffmpeg_pthread_create(&handle->monitor_pid, NULL, curl_ffmpeg_interrupt_monitor_thread, handle);
    //pthread_setname_np(handle->monitor_pid, "curlffmpeg-monitor");
    */
    ret = curl_fetch_open(handle->cfc_h);
    if(ret) {
        curl_fetch_close(handle->cfc_h);
        av_free(handle);
        handle = NULL;
        return ret;
    }
    /*
    handle->monitor_quited = 1;
    ffmpeg_pthread_join(handle->monitor_pid, NULL);
    */
    h->http_code = handle->cfc_h->http_code;
    h->is_slowmedia = 1;
    h->is_streamed = handle->cfc_h->seekable ? 0 : 1;
    h->location = handle->cfc_h->relocation;
    h->priv_data = handle;
    return ret;
}

static int curl_ffmpeg_read(URLContext *h, uint8_t *buf, int size)
{
    int ret = -1;
    CURLFFContext * s = (CURLFFContext *)h->priv_data;
    if (!s) {
        CLOGE("CURLFFContext invalid\n");
        return ret;
    }
    int counts = 200;
#if 1
    do {
        if (url_interrupt_cb()) {
            return AVERROR(EINTR);
        }
        ret = curl_fetch_read(s->cfc_h, buf, size);
        if (ret == C_ERROR_EAGAIN) {
            usleep(10 * 1000);
        }
        if (ret >= 0 || ret == C_ERROR_UNKNOW) {
            break;
        }
        /* just temporary, need to modify later */
        if (ret < C_ERROR_EAGAIN) {
            ret = AVERROR(ENETRESET);
            break;
        }
    } while (counts-- > 0);
#else
    ret = curl_fetch_read(s->cfc_h, buf, size);
#endif


#if 0
    FILE * fp = fopen("/temp/curl_dump.dat", "ab+");
    if (fp) {
        fwrite(buf, 1, ret, fp);
        fflush(fp);
        fclose(fp);
    }
#endif

    return ret;
}

static int64_t curl_ffmpeg_seek(URLContext *h, int64_t off, int whence)
{
    CLOGI("curl_ffmpeg_seek enter\n");
    int ret = -1;
    CURLFFContext * s = (CURLFFContext *)h->priv_data;
    if (!s) {
        CLOGE("CURLFFContext invalid\n");
        return ret;
    }
    if (!s->cfc_h) {
        CLOGE("CURLFFContext invalid CFContext handle\n");
        return ret;
    }
    if (whence == AVSEEK_CURL_HTTP_KEEPALIVE) {
        /*
        s->monitor_quited = 0;
        ffmpeg_pthread_create(&s->monitor_pid, NULL, curl_ffmpeg_interrupt_monitor_thread, s);
        //pthread_setname_np(s->monitor_pid, "curlffmpeg-monitor");
        */
        ret = curl_fetch_http_keepalive_open(s->cfc_h, NULL);
        //s->monitor_quited = 1;
        //ffmpeg_pthread_join(s->monitor_pid, NULL);
    } else if (whence == AVSEEK_SIZE) {
        ret = s->cfc_h->filesize;
    } else {
        ret = curl_fetch_seek(s->cfc_h, off, whence);
    }
    return ret;
}

static int curl_ffmpeg_close(URLContext *h)
{
    CLOGI("curl_ffmpeg_close enter\n");
    CURLFFContext * s = (CURLFFContext *)h->priv_data;
    if (!s) {
        return -1;
    }
    curl_fetch_close(s->cfc_h);
    av_free(s);
    s = NULL;
    return 0;
}

#if 0
static void * curl_ffmpeg_interrupt_monitor_thread(void *_handle)
{
    CURLFFContext * h = (CURLFFContext *)_handle;
    if(!h) {
        return NULL;
    }
    while(!h->monitor_quited) {
        if(url_interrupt_cb()) {
            curl_fetch_interrupt(h->cfc_h);
            return NULL;
        }
        usleep(SLEEP_TIME_UNIT);
    }
    return NULL;
}
#endif

static int curl_ffmpeg_get_info(URLContext *h, uint32_t  cmd, uint32_t flag, int64_t *info)
{
    CURLFFContext * s = (CURLFFContext *)h->priv_data;
    if (!s) {
        return -1;
    }
    int ret = 0;
    if(cmd == AVCMD_GET_NETSTREAMINFO) {
        if(flag == 1) {
            int64_t tmp_info = 0;
            ret = curl_fetch_get_info(s->cfc_h, C_INFO_SPEED_DOWNLOAD, flag, (void * )&tmp_info);
            if(!ret) {
                *info = tmp_info;
            } else {
                *info = 0;
            }
        }
    }
    return 0;
}

URLProtocol ff_curl_protocol = {
    .name               = "curl",
    .url_open           = curl_ffmpeg_open,
    .url_read           = curl_ffmpeg_read,
    .url_write          = NULL,
    .url_seek           = curl_ffmpeg_seek,
    //.url_exseek           = curl_ffmpeg_seek,
    .url_close          = curl_ffmpeg_close,
    .url_getinfo            = curl_ffmpeg_get_info,
    .url_get_file_handle    = NULL,
    .url_check          = NULL,
    //.priv_data_size       = sizeof(CURLFFContext),
    //.priv_data_class      = &curlffmpeg_class,
};