/*
 * MMS protocol over HTTP
 * Copyright (c) 2010 Zhentan Feng <spyfeng at gmail dot com>
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

/*
 * Reference
 * Windows Media HTTP Streaming Protocol.
 * http://msdn.microsoft.com/en-us/library/cc251059(PROT.10).aspx
 */

#include <string.h>
#include "libavutil/intreadwrite.h"
#include "libavutil/avstring.h"
#include "internal.h"
#include "mms.h"
#include "asf.h"
#include "http.h"
#include "url.h"
#include "mmsh.h"



#define CHUNK_HEADER_LENGTH 4   // 2bytes chunk type and 2bytes chunk length.
#define EXT_HEADER_LENGTH   8   // 4bytes sequence, 2bytes useless and 2bytes chunk length.

// see Ref 2.2.1.8
//#define USERAGENT  "User-Agent: NSPlayer/4.1.0.3856\r\n"
#define USERAGENT  "User-Agent: NSPlayer/12.0.7680.0\r\n"

// see Ref 2.2.1.4.33
// the guid value can be changed to any valid value.
#define CLIENTGUID "Pragma: xClientGUID={c77e7400-738a-11d2-9add-0020af0a3278}\r\n"

// see Ref 2.2.3 for packet type define:
// chunk type contains 2 fields: Frame and PacketID.
// Frame is 0x24 or 0xA4(rarely), different PacketID indicates different packet type.

static const char* mmsh_FirstRequest =   
	"Accept: */*\r\n"
	USERAGENT
	"Host: %s:%d\r\n"
	"Progma:version11-enabled=1\r\n"
	"Pragma: no-cache,rate=1.000000,stream-time=0,stream-offset=0:0,pacakge-num=4294967295,request-context=%u,max-duration=0\r\n"
	CLIENTGUID
	"Connection: Close\r\n\r\n";

static const char* mmsh_SeekableRequest =    
 	"Accept: */*\r\n"
	USERAGENT
	"Host: %s:%d\r\n"
	"Pragma: no-cache,rate=1.000000,request-context=%u,max-duration=%u\r\n"
	"Pragma: xPlayStrm=1\r\n"
	CLIENTGUID
	"Pragma: stream-switch-count=%d\r\n"
	"Pragma: stream-switch-entry=%s\r\n"
	"Pragma: no-cache,rate=1.000000,stream-time=%u\r\n"
	"Connection: Close\r\n\r\n";

static const char* mmsh_LiveRequest =   
	"Accept: */*\r\n"
	USERAGENT
	"Host: %s:%d\r\n"
	"Pragma: no-cache,rate=1.000000,request-context=%u\r\n"
	"Pragma: xPlayStrm=1\r\n"
	CLIENTGUID
	"Pragma: stream-switch-count=%d\r\n"
	"Pragma: stream-switch-entry=%s\r\n"
	"Connection: Close\r\n\r\n";

typedef enum {
    CHUNK_TYPE_DATA          = 0x4424,
    CHUNK_TYPE_ASF_HEADER    = 0x4824,
    CHUNK_TYPE_END           = 0x4524,
    CHUNK_TYPE_STREAM_CHANGE = 0x4324,
    CHUNK_TYPE_METADATA = 0x4D24,
} ChunkType;

typedef struct {
    MMSContext mms;
    uint8_t location[1024];
    int request_seq;  ///< request packet sequence
    int chunk_seq;    ///< data packet sequence
    int flags;
} MMSHContext;

typedef enum {
    MMSH_UNKNOW = 0,
    MMSH_SEEKABLE,
    MMSH_LIVE,
} StreamType;

static int mmsh_close(URLContext *h)
{
    MMSHContext *mmsh = (MMSHContext *)h->priv_data;
    if(NULL!=mmsh)	{
        MMSContext *mms   = &mmsh->mms;
        if (mms->mms_hd)
            ffurl_close(mms->mms_hd);
        av_free(mms->streams);
        av_free(mms->asf_header);
        av_freep(&h->priv_data);

    }
    return 0;
}

static ChunkType get_chunk_header(MMSHContext *mmsh, int *len)
{
    MMSContext *mms = &mmsh->mms;
    uint8_t chunk_header[CHUNK_HEADER_LENGTH];
    uint8_t ext_header[EXT_HEADER_LENGTH];
    ChunkType chunk_type;
    int chunk_len, res, ext_header_len;

    res = ffurl_read_complete(mms->mms_hd, chunk_header, CHUNK_HEADER_LENGTH);
    if (res != CHUNK_HEADER_LENGTH) {
        av_log(NULL, AV_LOG_ERROR, "Read data packet header failed!\n");
        return AVERROR(EIO);
    }
    chunk_type = AV_RL16(chunk_header);
    chunk_len  = AV_RL16(chunk_header + 2);
    chunk_type &=0xff7f;//never care B flag in framing header,see 2.2.3.1.1
    av_log(NULL, AV_LOG_INFO, " chunk type 0x%x,chunk len:%d\n", chunk_type,chunk_len);
    switch (chunk_type&0xff7f) {
    case CHUNK_TYPE_END:
    case CHUNK_TYPE_STREAM_CHANGE:
        ext_header_len = 4;
        break;
    case CHUNK_TYPE_ASF_HEADER:
    case CHUNK_TYPE_DATA:
        ext_header_len = 8;
        break;
    case CHUNK_TYPE_METADATA:
	 ext_header_len = 8;	
	 break;
    default:
        av_log(NULL, AV_LOG_ERROR, "Strange chunk type %d\n", chunk_type);
        ext_header_len = 0;
        break;
        //return AVERROR_INVALIDDATA;
    }

    res = ffurl_read_complete(mms->mms_hd, ext_header, ext_header_len);
    if (res != ext_header_len) {
        av_log(NULL, AV_LOG_ERROR, "Read ext header failed!\n");
        return AVERROR(EIO);
    }
    *len = chunk_len - ext_header_len;
    if (chunk_type == CHUNK_TYPE_END || chunk_type == CHUNK_TYPE_DATA)
        mmsh->chunk_seq = AV_RL32(ext_header);

    if(chunk_type == CHUNK_TYPE_END){
        av_log(NULL,AV_LOG_INFO,"Reason for End of Stream notificatation Packet,value :0x%x\n",mmsh->chunk_seq);
    }
    return chunk_type;
}

static int read_data_packet(MMSHContext *mmsh, const int len)
{
    MMSContext *mms   = &mmsh->mms;
    int res;
    if (len > sizeof(mms->in_buffer)) {
        av_log(NULL, AV_LOG_ERROR,
               "Data packet length %d exceeds the in_buffer size %zu\n",
               len, sizeof(mms->in_buffer));
        return AVERROR(EIO);
    }
    res = ffurl_read_complete(mms->mms_hd, mms->in_buffer, len);
    av_dlog(NULL, "Data packet len = %d\n", len);
    if (res != len) {
        av_log(NULL, AV_LOG_ERROR, "Read data packet failed!\n");
        return AVERROR(EIO);
    }
    if (len > mms->asf_packet_len) {
        av_log(NULL, AV_LOG_ERROR,
               "Chunk length %d exceed packet length %d\n",len, mms->asf_packet_len);
        return AVERROR_INVALIDDATA;
    } else {
        memset(mms->in_buffer + len, 0, mms->asf_packet_len - len); // padding
    }
    mms->read_in_ptr      = mms->in_buffer;
    mms->remaining_in_len = mms->asf_packet_len;
    return 0;
}

static int get_http_header_data(MMSHContext *mmsh)
{
    MMSContext *mms = &mmsh->mms;
    int res, len;
    ChunkType chunk_type;

    for (;;) {
        len = 0;
        res = chunk_type = get_chunk_header(mmsh, &len);
        if (res < 0) {
            return res;
        } else if (chunk_type == CHUNK_TYPE_ASF_HEADER){
            // get asf header and stored it
            if (!mms->header_parsed) {
                if (mms->asf_header) {
                    if (len != mms->asf_header_size) {
                        mms->asf_header_size = len;
                        av_dlog(NULL, "Header len changed from %d to %d\n",
                                mms->asf_header_size, len);
                        av_freep(&mms->asf_header);
                    }
                }
                mms->asf_header = av_mallocz(len);
                if (!mms->asf_header) {
                    return AVERROR(ENOMEM);
                }
                mms->asf_header_size = len;
            }
            if (len > mms->asf_header_size) {
                av_log(NULL, AV_LOG_ERROR,
                       "Asf header packet len = %d exceed the asf header buf size %d\n",
                       len, mms->asf_header_size);
                return AVERROR(EIO);
            }
            res = ffurl_read_complete(mms->mms_hd, mms->asf_header, len);
            if (res != len) {
                av_log(NULL, AV_LOG_ERROR,
                       "Recv asf header data len %d != expected len %d\n", res, len);
                return AVERROR(EIO);
            }
            mms->asf_header_size = len;
            if (!mms->header_parsed) {
                res = ff_mms_asf_header_parser(mms);
                mms->header_parsed = 1;
                return res;
            }
        } else if (chunk_type == CHUNK_TYPE_DATA) {
            // read data packet and do padding
            return read_data_packet(mmsh, len);
        } else {
            if (len) {
                if (len > sizeof(mms->in_buffer)) {
                    av_log(NULL, AV_LOG_ERROR,
                           "Other packet len = %d exceed the in_buffer size %zu\n",
                           len, sizeof(mms->in_buffer));
                    return AVERROR(EIO);
                }
                res = ffurl_read_complete(mms->mms_hd, mms->in_buffer, len);
                if (res != len) {
                    av_log(NULL, AV_LOG_ERROR, "Read other chunk type data failed!\n");
                    return AVERROR(EIO);
                } else {
                    av_log(NULL,AV_LOG_WARNING,"Skip chunk type %d \n", chunk_type);
                    continue;
                }
            }
        }
    }
    return 0;
}

static int mmsh_open_internal(URLContext *h, const char *uri, int flags, int timestamp, int64_t pos)
{
    int i, port, err;
    char httpname[256], path[256], host[128];
    char *stream_selection = NULL;
    char headers[1024];
    MMSHContext *mmsh;
    MMSContext *mms;	
    StreamType stype = MMSH_SEEKABLE;
    mmsh = h->priv_data = av_mallocz(sizeof(MMSHContext));     
   
	
    if (!h->priv_data)
        return AVERROR(ENOMEM);

    mmsh->request_seq = h->is_streamed = 1;
		
    mms = &mmsh->mms;
	
    if(!strncmp(uri,"mmsh:shttp://",strlen("mmsh:shttp://")))/*protocol swtich,add the mmsh on the header*/
        av_strlcpy(mmsh->location, uri+6, sizeof(mmsh->location));//del the mmsh first   
    else		
	 av_strlcpy(mmsh->location, uri, sizeof(mmsh->location));
	 
    
   
    av_url_split(NULL, 0, NULL, 0,
        host, sizeof(host), &port, path, sizeof(path), mmsh->location);   
    if (port<0)
        port = 80; // default mmsh protocol port
    ff_url_join(httpname, sizeof(httpname), "http", NULL, host, port, "%s", path);

    if (ffurl_alloc(&mms->mms_hd, httpname, AVIO_FLAG_READ) < 0) {
        return AVERROR(EIO);
    }  
    snprintf(headers, sizeof(headers),
             mmsh_FirstRequest,
             host, port, mmsh->request_seq++);
    ff_http_set_headers(mms->mms_hd, headers);
		   
    err = ffurl_connect(mms->mms_hd);
    if (err) {
        goto fail;
    }

    err = get_http_header_data(mmsh);
    if (err) {
        av_log(NULL, AV_LOG_ERROR, "Get http header data failed!\n");
        goto fail;
    }
    
    if(mms->mms_hd->prot){
        int iflag = ff_http_get_broadcast_flag(mms->mms_hd);
        
     
        if(iflag>0){
            av_log(NULL,AV_LOG_INFO,"Get a broadcast stream flag\n");
            stype = MMSH_LIVE;
        }else{
            av_log(NULL,AV_LOG_INFO,"Get a seekable stream flag\n");
            stype = MMSH_SEEKABLE;
        }
        
        // close the socket and then reopen it for sending the second play request.
        ffurl_close(mms->mms_hd);
    }
    //fix me? just need probe data,for judge live and vod.	
    memset(headers, 0, sizeof(headers));
    if ((err = ffurl_alloc(&mms->mms_hd, httpname, AVIO_FLAG_READ)) < 0) {
        goto fail;
    }
    stream_selection = av_mallocz(mms->stream_num * 19 + 1);
    if (!stream_selection)
        return AVERROR(ENOMEM);
    for (i = 0; i < mms->stream_num; i++) {
        char tmp[20];
        err = snprintf(tmp, sizeof(tmp), "ffff:%d:0 ", mms->streams[i].id);
        if (err < 0)
            goto fail;
        av_strlcat(stream_selection, tmp, mms->stream_num * 19 + 1);
    }
    // send play request

    if(stype ==MMSH_SEEKABLE){
        err = snprintf(headers, sizeof(headers),
                   mmsh_SeekableRequest,
                   host, port, mmsh->request_seq++, 0 ,mms->stream_num, stream_selection, timestamp);

    }else if(stype == MMSH_LIVE){
        err = snprintf(headers, sizeof(headers),
                   mmsh_LiveRequest,
                   host, port, mmsh->request_seq++,mms->stream_num, stream_selection);

    }
    av_freep(&stream_selection);
    if (err < 0) {
        av_log(NULL, AV_LOG_ERROR, "Build play request failed!\n");
        goto fail;
    }
    av_log(NULL,AV_LOG_DEBUG, "out_buffer is %s", headers);
    ff_http_set_headers(mms->mms_hd, headers);

    err = ffurl_connect(mms->mms_hd);
    if (err) {
          goto fail;
    }
    h->is_slowmedia=1;
    err = get_http_header_data(mmsh);
    if (err) {
        av_log(NULL, AV_LOG_ERROR, "Get http header data failed!\n");
        goto fail;
    }

    av_dlog(NULL, "Connection successfully open\n");
    return 0;
fail:
    av_freep(&stream_selection);
    mmsh_close(h);
    av_dlog(NULL, "Connection failed with error %d\n", err);
    return err;
}

static int mmsh_open(URLContext *h, const char *uri, int flags)
{
    return mmsh_open_internal(h, uri, flags, 0, 0);
}

static int handle_chunk_type(MMSHContext *mmsh)
{
    MMSContext *mms = &mmsh->mms;
    int res, len = 0;
    ChunkType chunk_type;
    chunk_type = get_chunk_header(mmsh, &len);

    switch (chunk_type) {
    case CHUNK_TYPE_END:
        mmsh->chunk_seq = 0;
        av_log(NULL, AV_LOG_ERROR, "Stream ended!\n");
        return AVERROR(EIO);
    case CHUNK_TYPE_STREAM_CHANGE:
        mms->header_parsed = 0;
        if (res = get_http_header_data(mmsh)) {
            av_log(NULL, AV_LOG_ERROR,"Stream changed! Failed to get new header!\n");
            return res;
        }
        break;
    case CHUNK_TYPE_DATA:
        return read_data_packet(mmsh, len);
    default:
        av_log(NULL, AV_LOG_ERROR, "Recv other type packet %d\n", chunk_type);
        return AVERROR_INVALIDDATA;
    }
    return 0;
}

static int mmsh_read(URLContext *h, uint8_t *buf, int size)
{
    int res = 0;
    MMSHContext *mmsh = h->priv_data;
    MMSContext *mms   = &mmsh->mms;
    int retry=10;	
    do {
        if (mms->asf_header_read_size < mms->asf_header_size) {
            // copy asf header into buffer
            res = ff_mms_read_header(mms, buf, size);
        } else {
            if (!mms->remaining_in_len && (res = handle_chunk_type(mmsh))){
              	if(retry--<0)/*for loop play,we will get CHUNK_TYPE_END on read.*/
				return res;
			continue;	
            }
            res = ff_mms_read_data(mms, buf, size);
        }
    } while (!res);
    return res;
}
int is_mmsh_file(AVIOContext *pb,const char *name)
{
	int score=0;
	char line[1024+1];
	int ret;
	int linecnt=0;
	if(!pb) return 0;	
	do
	{	
		ret=ff_get_assic_line(pb,line,1024);		
		if(ret==0){
			if(url_feof(pb) || url_ferror(pb))
				return 0;
			}else if(ret<0)
				break;
		if(ret<5) continue;      	
		if(!strncmp(line,"[Reference]",strlen("[Reference]"))){
			score+=50;
			continue;
		}else if(!strncmp(line,"Ref1=",strlen("Ref1="))){
			score+=30;
			if(strstr(line,"MSWM.asf"))
				score+=30;
		}
		else if(!strncmp(line,"Ref2=",strlen("Ref2="))){
			score+=30;
			if(strstr(line,"MSWM.asf"))
				score+=30;
		}
	}while(score>=0 && score<100 && ret>0 && linecnt++<5);
	av_log(NULL,AV_LOG_INFO,"is_mmsh_file=%d\n",score);
	return score>=100?100:score;
}

static int64_t mmsh_read_seek(URLContext *h, int stream_index,
                        int64_t timestamp, int flags)
{
	MMSHContext *mmsh = h->priv_data;
	MMSContext *mms   = &mmsh->mms;
	int ret;
	
	//av_log(NULL,AV_LOG_DEBUG,"%s======= %d,%lld,%d,%d\n", __FUNCTION__,__LINE__,timestamp,flags,stream_index);
	ret= mmsh_open_internal(h, mmsh->location, 0, FFMAX(timestamp, 0), 0);

	if(ret>=0){
		if (mms->mms_hd)
		    ffurl_close(mms->mms_hd);
		av_freep(&mms->streams);
		av_freep(&mms->asf_header);
		av_free(mmsh);
		mmsh = h->priv_data;
		mms   = &mmsh->mms;
		mms->asf_header_read_size= mms->asf_header_size;
	}else
		h->priv_data= mmsh;
	return ret;
}

static int64_t mmsh_seek(URLContext *h, int64_t pos, int whence)
{
	//av_log(NULL,AV_LOG_DEBUG, "%s======= %d,%lld,%d\n", __FUNCTION__,__LINE__,pos,whence);
	MMSHContext *mmsh = h->priv_data;
	MMSContext *mms   = &mmsh->mms;
	
	if (whence == AVSEEK_SIZE&&(mms->flags&0x02>0)&&(mms->flags&0x01==0)) {
		av_log(NULL,AV_LOG_DEBUG,"[%s]:Get stream size: %lld\n",mms->file_size);
		return mms->file_size;
	}
	if(pos == 0 && whence == SEEK_CUR)
		return mms->asf_header_read_size + mms->remaining_in_len + mmsh->chunk_seq * mms->asf_packet_len;
	return AVERROR(ENOSYS);
}
URLProtocol ff_mmsh_protocol = {
    .name      = "mmsh",
    .url_open  = mmsh_open,
    .url_read  = mmsh_read,
    .url_seek  = mmsh_seek,
    .url_close = mmsh_close,
    .url_read_seek  = mmsh_read_seek,  
};
