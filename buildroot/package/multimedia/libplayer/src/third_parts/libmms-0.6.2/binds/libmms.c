/*
 * libmms protocol
 * Copyright (c) amlogic,2012 peter Chu
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


#include "libavutil/avstring.h"
#include "libavformat/avformat.h"
#include "libavformat/url.h"
#include "libavformat/internal.h"

#include "mmsx.h"

#define DEFAULT_CONNECTION_SPEED    0
#ifndef INT_MAX
#define INT_MAX   2147483647
#endif

typedef struct _MMSXContext{
    char uri_name[MAX_URL_SIZE];
    mmsx_t *connection;
    int64_t connection_speed;
    int seekable;
}MMSXContext;

#define MMS_TAG "libmms"

#define MLOG(...) av_tag_log(MMS_TAG,__VA_ARGS__)




static int ff_mmsx_open(URLContext *h, const char *uri, int flags)
{
    if(uri == NULL||strlen(uri)<1){
        MLOG("Invalid mmsx url path\n");
        return -1;
    }
    int ret = -1;
    MMSXContext*  handle = NULL;
    handle = av_mallocz(sizeof(MMSXContext));
    if(handle == NULL){
        MLOG("Failed to allocate memory for mmsx handle\n");
        return -1;
    }
    handle->connection_speed = FFMAX(DEFAULT_CONNECTION_SPEED,INT_MAX/1000);

    int bandwidth_avail = handle->connection_speed;

    memset(handle->uri_name,0,sizeof(handle->uri_name));
    if(!strncmp(uri,"mmsx:shttp://",strlen("mmsx:shttp://"))){/*protocol swtich,add the mmsh on the header*/
        snprintf(handle->uri_name,5,"mmsh");
        snprintf(handle->uri_name+4,MAX_URL_SIZE-4,"%s",uri+10);//delhandle->uri_name the mmsh first   

    }
    else		
	 av_strlcpy(handle->uri_name, uri, sizeof(handle->uri_name));
    
    MLOG("Trying mmsx_connect (%s) with bandwidth constraint of %d bps",handle->uri_name, bandwidth_avail);   
    handle->connection = mmsx_connect (NULL, NULL, handle->uri_name, bandwidth_avail);
    if(NULL == handle->connection){//redirect to rtsp
#if 0    
        char *url, *location;
        MLOG("Could not connect to this stream, redirecting to rtsp\n");
        location = strstr (handle->uri_name, "://");
        if (location == NULL || *location == '\0' || *(location + 3) == '\0'){
            av_free(handle->uri_name);
            av_free(handle);
            return -1;
        }
        url = av_mallocz(strlen(location)+strlen("rtsp://"));
        if(url == NULL){
            av_free(handle->uri_name);
            av_free(handle);
            return -1;
        }
        
        snprintf(url,MAX_URL_SIZE,"rtsp://%s", location + 3);        
        //do redirect,...TBD.
        av_free(handle);       
        return 0;
#endif
        return -1;
        
    }
    
    h->is_slowmedia = 1;
    h->is_streamed = 1;

    int is_seekable = mmsx_get_seekable(handle->connection);
    handle->seekable =0;
    if (is_seekable > 0) {
        h->support_time_seek = 1;
        handle->seekable =1;
    }
    h->priv_data = handle;   
    return 0;   
}


static int ff_mmsx_read(URLContext *s, uint8_t *buf, int size)
{
    MMSXContext*  handle = (MMSXContext*)s->priv_data;
    if(handle == NULL){
        MLOG("Failed to get mmsx handle\n");
        return -1;
    }
    
    int ret = -1;
    char* data;
    int blocksize;
    mms_off_t offset;
    int retires = 10;
    int rsize =0;
    do{      
        offset = mmsx_get_current_pos (handle->connection);
        /* Check if a seek perhaps has wrecked our connection */
        if (offset == -1) {
            MLOG("connection broken (probably an error during mmsx_seek_time during a convert query) returning FLOW_ERROR\n");
            return -1;
        }
        if(offset == 0){
            blocksize = mmsx_get_asf_header_len (handle->connection);
        }else{
            blocksize = mmsx_get_asf_packet_len (handle->connection);
        }
        MLOG ("Reading %d bytes", blocksize);

        rsize = FFMIN(blocksize,size);
        if (url_interrupt_cb()) {
            break;
        }
        ret = mmsx_read (NULL, handle->connection, (char *) buf, rsize);
        if (url_interrupt_cb()) {           
            break;
        }

    }while(!ret&&retires-->0);
    /* EOS? */  
    if (ret == 0){
        MLOG ("Just Can't get data,maybe EOS");
        return 0;
    }        
    
    return ret;   
}


static int64_t ff_mmsx_read_seek(URLContext *s, int stream_index,
                              int64_t timestamp, int flags){
    MMSXContext*  handle = (MMSXContext*)s->priv_data;
    if(handle == NULL){
        MLOG("Failed to get mmsx handle\n");
        return -1;
    }

    if (flags & AVSEEK_FLAG_BYTE)
        return AVERROR(ENOSYS);

    /* seeks are in milliseconds */
    if (stream_index < 0)
        timestamp = av_rescale_rnd(timestamp, 1000, AV_TIME_BASE,
            flags & AVSEEK_FLAG_BACKWARD ? AV_ROUND_DOWN : AV_ROUND_UP);

    if (!mmsx_time_seek(NULL,handle->connection, timestamp/1000))
        return -1;
    return timestamp;


}

static int64_t ff_mmsx_seek(URLContext *h, int64_t pos, int whence){
    MMSXContext*  handle = (MMSXContext*)h->priv_data;
    if(handle == NULL){
        MLOG("Failed to get mmsx handle\n");
        return -1;
    }
    

    if(handle->seekable<1){
        return -1;
    }
    int64_t value = -1;
    if(whence == AVSEEK_SIZE){

        value = (int64_t) mmsx_get_length (handle->connection);       
    }else if(whence == SEEK_SET){
        value = mmsx_seek (NULL, handle->connection, pos, SEEK_SET);
    }
    return value;

}

static int64_t ff_mmsx_time_seek(URLContext *h, int64_t time_sec, int whence){
    MMSXContext*  handle = (MMSXContext*)h->priv_data;
    if(handle == NULL){
        MLOG("Failed to get mmsx handle\n");
        return -1;
    }
    
    int ret = -1;
    if(handle->seekable<1||time_sec<0){
        return -1;
    }
    int64_t st = 0;
    if (whence == AVSEEK_FULLTIME) {      
         st = (int64_t)mmsx_get_time_length (handle->connection);
         MLOG("Get mmsx total time:%lld\n",st);
         return st;
    
    }else if(whence == AVSEEK_TO_TIME){
        ret = mmsx_time_seek(NULL,handle->connection,(double)time_sec);
    }
    
    return ret;
}

static int ff_mmsx_close(URLContext *s)
{
    MMSXContext*  handle = (MMSXContext*)s->priv_data;
    if(handle == NULL){
        MLOG("Failed to get mmsx handle\n");
        return -1;
    }
    int ret = -1;
    //mmsx_abort(handle->connection);  
    mmsx_close (handle->connection);    
    handle->connection = NULL;    
    av_free(handle);
    handle = NULL;
    return 0;  
}
URLProtocol ff_mmsx_protocol = {
    .name                = "mmsx",
    .url_open            = ff_mmsx_open,
    .url_read            = ff_mmsx_read,    
    .url_close           = ff_mmsx_close,
    .url_seek           = ff_mmsx_seek,
    .url_read_seek    = ff_mmsx_read_seek,
    .url_exseek         = ff_mmsx_time_seek,
};

