/*
 * RTMP network protocol
 * Copyright (c) 2010 Howard Chu
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

/**
 * @file
 * RTMP protocol based on http://rtmpdump.mplayerhq.hu/ librtmp
 */

#include "avformat.h"
#include "url.h"
#include "libavformat/avio.h"

#include <unistd.h>
#include <librtmp/rtmp.h>
#include <librtmp/log.h>

#define DEF_BUFTIME	(10 * 60 * 60 * 1000)	/* 10 hours default */
#define DEF_SKIPFRM	0
#define DEF_TIMEOUT	30	/* seconds */

static void rtmp_log(int level, const char *fmt, va_list args)
{
    switch (level) {
    default:
    case RTMP_LOGCRIT:    level = AV_LOG_FATAL;   break;
    case RTMP_LOGERROR:   level = AV_LOG_ERROR;   break;
    case RTMP_LOGWARNING: level = AV_LOG_WARNING; break;
    case RTMP_LOGINFO:    level = AV_LOG_INFO;    break;
    case RTMP_LOGDEBUG:   level = AV_LOG_VERBOSE; break;
    case RTMP_LOGDEBUG2:  level = AV_LOG_DEBUG;   break;
    }

    av_vlog(NULL, level, fmt, args);
    av_log(NULL, level, "\n");
}

static int rtmp_setupURL(RTMP *r, const char *uri)
{
    double percent = 0;
    double duration = 0.0;   
    int nSkipKeyFrames = DEF_SKIPFRM; 
    int bOverrideBufferTime = FALSE;
    uint32_t dSeek = 0;
    uint32_t bufferTime = DEF_BUFTIME;
    
    // meta header and initial frame for the resume mode (they are read from the file and compared with
    // the stream we are trying to continue
    char *metaHeader = 0;
    uint32_t nMetaHeaderSize = 0;
    
    // video keyframe for matching
    char *initialFrame = 0;
    uint32_t nInitialFrameSize = 0;
    int initialFrameType = 0;	// tye: audio or video
    
    AVal hostname = { 0, 0 };
    AVal playpath = { 0, 0 };
    AVal subscribepath = { 0, 0 };
    AVal usherToken = { 0, 0 }; //Justin.tv auth token
    int port = -1;
    int protocol = RTMP_PROTOCOL_UNDEFINED;
    int retries = 0;
    int bLiveStream = FALSE;	// is it a live stream? then we can't seek/resume
    int bRealtimeStream = FALSE;  // If true, disable the BUFX hack (be patient)
    int bHashes = FALSE;		// display byte counters not hashes by default
    
    long int timeout = DEF_TIMEOUT;	// timeout connection after 120 seconds
    uint32_t dStartOffset = 0;	// seek position in non-live mode
    uint32_t dStopOffset = 0;
    
    AVal fullUrl = { 0, 0 };
    AVal swfUrl = { 0, 0 };
    AVal tcUrl = { 0, 0 };
    AVal pageUrl = { 0, 0 };
    AVal app = { 0, 0 };
    AVal auth = { 0, 0 };
    AVal swfHash = { 0, 0 };
    uint32_t swfSize = 0;
    AVal flashVer = { 0, 0 };
    AVal sockshost = { 0, 0 };
    
    #ifdef CRYPTO
     int swfAge = 30;	/* 30 days for SWF cache by default */
     int swfVfy = 0;
     unsigned char hash[RTMP_SWF_HASHLEN];
    #endif
     
    {
        AVal parsedHost, parsedApp, parsedPlaypath;
        unsigned int parsedPort = 0;
        int parsedProtocol = RTMP_PROTOCOL_UNDEFINED;
        
        if (!RTMP_ParseURL
        (uri, &parsedProtocol, &parsedHost, &parsedPort,
        &parsedPlaypath, &parsedApp))
        {
            av_log(NULL,AV_LOG_ERROR,"Couldn't parse the specified url (%s)!\n", uri);
        }
        else
        {
            if (!hostname.av_len)
                hostname = parsedHost;
            if (port == -1)
                port = parsedPort;
            if (playpath.av_len == 0 && parsedPlaypath.av_len)
            {
                playpath = parsedPlaypath;
            }
            if (protocol == RTMP_PROTOCOL_UNDEFINED)
                protocol = parsedProtocol;
            if (app.av_len == 0 && parsedApp.av_len)
            {
                app = parsedApp;
            }
            if(parsedApp.av_len && strstr(parsedApp.av_val, "live"))
            {
                bLiveStream = TRUE;
            }
        }
    }

    if (!hostname.av_len && !fullUrl.av_len)
    {
        av_log(NULL,AV_LOG_ERROR,"Couldn't parse hostname !\n");
        return FALSE;
    }
    if (!playpath.av_len && !fullUrl.av_len)
    {
        av_log(NULL,AV_LOG_ERROR,"Couldn't parse playpath !\n");
        return FALSE;
    }
    
    if (protocol == RTMP_PROTOCOL_UNDEFINED && !fullUrl.av_len)
    {
        protocol = RTMP_PROTOCOL_RTMP;
    }
    if (port == -1 && !fullUrl.av_len)
    {
        port = 0;
    }
    if (port == 0 && !fullUrl.av_len)
    {
        if (protocol & RTMP_FEATURE_SSL)
            port = 443;
        else if (protocol & RTMP_FEATURE_HTTP)
            port = 80;
        else
            port = 1935;
    }
    
    #ifdef CRYPTO
    if (swfVfy)
    {
        if (RTMP_HashSWF(swfUrl.av_val, &swfSize, hash, swfAge) == 0)
        {
            swfHash.av_val = (char *)hash;
            swfHash.av_len = RTMP_SWF_HASHLEN;
        }
    }
    
    if (swfHash.av_len == 0 && swfSize > 0)
    {
        swfSize = 0;
    }
    
    if (swfHash.av_len != 0 && swfSize == 0)
    {
        swfHash.av_len = 0;
        swfHash.av_val = NULL;
    }
    #endif
    
    if (tcUrl.av_len == 0)
    {
        tcUrl.av_len = strlen(RTMPProtocolStringsLower[protocol]) + hostname.av_len + app.av_len + sizeof("://:65535/");
        tcUrl.av_val = (char *) av_malloc(tcUrl.av_len);
        if (!tcUrl.av_val)
            return FALSE;
        tcUrl.av_len = snprintf(tcUrl.av_val, tcUrl.av_len, "%s://%.*s:%d/%.*s",
    	 RTMPProtocolStringsLower[protocol], hostname.av_len,
        hostname.av_val, port, app.av_len, app.av_val);
    }

    if (!fullUrl.av_len)
    {
        RTMP_SetupStream(r, protocol, &hostname, port, &sockshost, &playpath,
        &tcUrl, &swfUrl, &pageUrl, &app, &auth, &swfHash, swfSize,
        &flashVer, &subscribepath, &usherToken, dSeek, dStopOffset, bLiveStream, timeout);
    }
    else
    {
        if (RTMP_SetupURL(r, fullUrl.av_val) == FALSE)
        {
            av_log(NULL,AV_LOG_ERROR,"rtmp setupURL failed !\n");
            return FALSE;
        }
    }

#if 0
    /* Try to keep the stream moving if it pauses on us */
    if (!bLiveStream && !bRealtimeStream && !(protocol & RTMP_FEATURE_HTTP))
        r.Link.lFlags |= RTMP_LF_BUFX;
#endif

    //RTMP_SetBufferMS(r, bufferTime);
    return TRUE;
}

static int rtmp_close(URLContext *s)
{
    RTMP *r = s->priv_data;

    RTMP_Close(r);
    av_free(r);
    return 0;
}

/**
 * Open RTMP connection and verify that the stream can be played.
 *
 * URL syntax: rtmp://server[:port][/app][/playpath][ keyword=value]...
 *             where 'app' is first one or two directories in the path
 *             (e.g. /ondemand/, /flash/live/, etc.)
 *             and 'playpath' is a file name (the rest of the path,
 *             may be prefixed with "mp4:")
 *
 *             Additional RTMP library options may be appended as
 *             space-separated key-value pairs.
 */
static int rtmp_open(URLContext *s, const char *uri, int flags)
{
    RTMP *r;
    int rc;

    r = av_mallocz(sizeof(RTMP));
    if (!r)
        return AVERROR(ENOMEM);

    switch (av_log_get_level()) {
    default:
    case AV_LOG_FATAL:   rc = RTMP_LOGCRIT;    break;
    case AV_LOG_ERROR:   rc = RTMP_LOGERROR;   break;
    case AV_LOG_WARNING: rc = RTMP_LOGWARNING; break;
    case AV_LOG_INFO:    rc = RTMP_LOGINFO;    break;
    case AV_LOG_VERBOSE: rc = RTMP_LOGDEBUG;   break;
    case AV_LOG_DEBUG:   rc = RTMP_LOGDEBUG2;  break;
    }
    RTMP_LogSetLevel(rc);
    RTMP_LogSetCallback(rtmp_log);

    RTMP_Init(r);
    
    /*
    if (!RTMP_SetupURL(r, s->filename)) {
	 
        rc = -1;
        goto fail;
    }
    */

    if(!rtmp_setupURL(r, uri)) {
        rc = -1;
        goto fail;
    }

    if (flags & AVIO_FLAG_WRITE)
        RTMP_EnableWrite(r);

    if (!RTMP_Connect(r, NULL) || !RTMP_ConnectStream(r, 0)) {
        rc = -1;
        goto fail;
    }

    s->priv_data   = r;
    s->is_streamed = 1;
    s->is_slowmedia =1;
    av_log(NULL,AV_LOG_INFO,"rtmp open,url:%s\n",s->filename);		
    return 0;
fail:
    av_free(r);
    return rc;
}

static int rtmp_write(URLContext *s, const uint8_t *buf, int size)
{
    RTMP *r = s->priv_data;

    return RTMP_Write(r, buf, size);
}

static int rtmp_read(URLContext *s, uint8_t *buf, int size)
{
    RTMP *r = s->priv_data;
    int nRead = 0;
    int retries = 20;
    
    do {
        if (url_interrupt_cb()>0) {
            nRead = 0;
            break;
        }
        nRead = RTMP_Read(r, buf, size);
        if (nRead > 0) {
            break;
        } else {
            if (r->m_read.status == RTMP_READ_EOF) {
                nRead = 0;
                break;
            }
            if(nRead == 0){
                nRead = AVERROR(EAGAIN);
                usleep(1000*100);
            }else{
                break;
            }
        }    
    } while (retries-- > 0);
    
    return nRead;
}

static int rtmp_read_pause(URLContext *s, int pause)
{
    RTMP *r = s->priv_data;

    if (!RTMP_Pause(r, pause))
        return -1;
    return 0;
}

static int64_t rtmp_read_seek(URLContext *s, int stream_index,
                              int64_t timestamp, int flags)
{
    RTMP *r = s->priv_data;

    if (flags & AVSEEK_FLAG_BYTE)
        return AVERROR(ENOSYS);

    /* seeks are in milliseconds */
    if (stream_index < 0)
        timestamp = av_rescale_rnd(timestamp, 1000, AV_TIME_BASE,
            flags & AVSEEK_FLAG_BACKWARD ? AV_ROUND_DOWN : AV_ROUND_UP);

    if (!RTMP_SendSeek(r, timestamp))
        return -1;
    return timestamp;
}

static int rtmp_get_file_handle(URLContext *s)
{
    RTMP *r = s->priv_data;

    return RTMP_Socket(r);
}

URLProtocol ff_rtmp_protocol = {
    .name                = "rtmp",
    .url_open            = rtmp_open,
    .url_read            = rtmp_read,
    .url_write           = rtmp_write,
    .url_close           = rtmp_close,
    .url_read_pause      = rtmp_read_pause,
    .url_read_seek       = rtmp_read_seek,
    .url_get_file_handle = rtmp_get_file_handle
};

URLProtocol ff_rtmpt_protocol = {
    .name                = "rtmpt",
    .url_open            = rtmp_open,
    .url_read            = rtmp_read,
    .url_write           = rtmp_write,
    .url_close           = rtmp_close,
    .url_read_pause      = rtmp_read_pause,
    .url_read_seek       = rtmp_read_seek,
    .url_get_file_handle = rtmp_get_file_handle
};

URLProtocol ff_rtmpe_protocol = {
    .name                = "rtmpe",
    .url_open            = rtmp_open,
    .url_read            = rtmp_read,
    .url_write           = rtmp_write,
    .url_close           = rtmp_close,
    .url_read_pause      = rtmp_read_pause,
    .url_read_seek       = rtmp_read_seek,
    .url_get_file_handle = rtmp_get_file_handle
};

URLProtocol ff_rtmpte_protocol = {
    .name                = "rtmpte",
    .url_open            = rtmp_open,
    .url_read            = rtmp_read,
    .url_write           = rtmp_write,
    .url_close           = rtmp_close,
    .url_read_pause      = rtmp_read_pause,
    .url_read_seek       = rtmp_read_seek,
    .url_get_file_handle = rtmp_get_file_handle
};

URLProtocol ff_rtmps_protocol = {
    .name                = "rtmps",
    .url_open            = rtmp_open,
    .url_read            = rtmp_read,
    .url_write           = rtmp_write,
    .url_close           = rtmp_close,
    .url_read_pause      = rtmp_read_pause,
    .url_read_seek       = rtmp_read_seek,
    .url_get_file_handle = rtmp_get_file_handle
};
