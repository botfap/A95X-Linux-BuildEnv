/*
 * cmfvpb  io for ffmpeg system
 * Copyright (c) 2001 Fabrice Bellard
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
 *
 *cmf:combine file,mutifiles extractor;
 *cmfvpb:virtual pb for cmf dec
 * amlogic:2012.6.29
 */


#include "libavutil/intreadwrite.h"
#include "libavutil/avstring.h"
#include "libavutil/dict.h"
#include "avformat.h"
#include "avio_internal.h"
#include "riff.h"
#include "isom.h"
#include "libavcodec/get_bits.h"
#include "avio.h"
#include <stdio.h>

#include "cmfvpb.h"


static int cmfvpb_open(struct cmfvpb *cv , int *index)
{
    int ret = 0;
    if (!cv ||  !cv->input || !cv->input->prot->url_seek || !cv->input->prot->url_getinfo) {
        av_log(NULL, AV_LOG_INFO, "cmfvpb_open is NULL\n");
        return -1;
    }
    ret = cv->input->prot->url_getinfo(cv->input, AVCMD_HLS_STREAMTYPE, 0 , &cv->hls_stream_type);
    ret = cv->input->prot->url_seek(cv->input, (int64_t)(*index), AVSEEK_SLICE_BYINDEX);
    if(!cv->hls_stream_type)
        *index = ret;
    if (ret < 0){
        av_log(NULL, AV_LOG_INFO, "seek failed %s---%d \n",__FUNCTION__,__LINE__);
        return -1;
    } 
    ret = cv->input->prot->url_getinfo(cv->input, AVCMD_TOTAL_DURATION, 0, &cv->total_duration); 
    ret = cv->input->prot->url_getinfo(cv->input, AVCMD_TOTAL_NUM, 0, &cv->total_num);
    ret = cv->input->prot->url_getinfo(cv->input, AVCMD_SLICE_START_OFFSET, 0, &cv->start_offset); 
    ret = cv->input->prot->url_getinfo(cv->input, AVCMD_SLICE_SIZE, 0 , &cv->size);
    ret = cv->input->prot->url_getinfo(cv->input, AVCMD_SLICE_STARTTIME, 0 , &cv->start_time);
    ret = cv->input->prot->url_getinfo(cv->input, AVCMD_SLICE_ENDTIME, 0 , &cv->end_time);
    
    if(cv->start_offset <= 0)
        cv->start_offset = 0;
    if(cv->size <= 0)
        cv->size =0;
    cv->end_offset = cv->start_offset + cv->size;
    cv->curslice_duration=cv->end_time-cv->start_time;
    cv->seg_pos = 0;
    cv->cur_index = *index;
    av_log(NULL, AV_LOG_INFO, "cmf  vpb_TOTALINFO  index[%lld] total_num [%lld] total_duration [%lld] \n",cv->cur_index,cv->total_num, cv->total_duration);
    av_log(NULL, AV_LOG_INFO, "cmf  vpb_SIZEINFO    index[%lld]  start_offset [%llx] end_offset [%llx] slice_size[%llx] \n", cv->cur_index, cv->start_offset, cv->end_offset, cv->size);
    av_log(NULL, AV_LOG_INFO, "cmf  vpb_TIMEINFO    index[%lld]  start_time[%lld]ms end_time[%lld]ms curslice_duration[%lld]ms\n", cv->cur_index, cv->start_time, cv->end_time, cv->curslice_duration);
    return ret;
}
int cmfvpb_getinfo(struct cmfvpb *cv, int cmd,int flag,int64_t*info)
{
    if (!cv || ! cv->input   || !cv->input->prot->url_getinfo) {
        av_log(NULL,AV_LOG_INFO,"url_get info NULL\n");
        return -1;
    }
    int ret ;
    int tmpcmd;
    int64_t *outinfo;
    switch(cmd){
            case AVCMD_TOTAL_DURATION:
                   outinfo= &cv->total_duration; 
                   tmpcmd=AVCMD_TOTAL_DURATION;
                   break;
            case AVCMD_TOTAL_NUM:
                   outinfo= &cv->total_num; 
                   tmpcmd=AVCMD_TOTAL_NUM;
                   break;
            case AVCMD_SLICE_START_OFFSET:
                   outinfo= &cv->start_offset; 
                   tmpcmd=AVCMD_SLICE_START_OFFSET;
                   break;
            case AVCMD_SLICE_SIZE:
                   outinfo= &cv->size; 
                   tmpcmd=AVCMD_SLICE_SIZE;
                   break;       
           case AVCMD_SLICE_STARTTIME:
                   outinfo= &cv->start_time; 
                   tmpcmd=AVCMD_SLICE_STARTTIME;
                   break;
            case AVCMD_SLICE_ENDTIME:
                   outinfo= &cv->end_time; 
                   tmpcmd=AVCMD_SLICE_ENDTIME;
                   break;      
            case AVCMD_SLICE_INDEX:
                   outinfo= &cv->cur_index; 
                   tmpcmd=AVCMD_SLICE_INDEX;
                   break;             
                   
            default:
				return 0; 
    }

    ret = cv->input->prot->url_getinfo(cv->input, tmpcmd, flag, outinfo); 
    if (ret < 0){
        av_log(NULL,AV_LOG_INFO,"url_get info errorcmd [%d] [%lld] \n",tmpcmd,*outinfo);
        return -1;
    }else {
        *info = *outinfo;
       // av_log(NULL,AV_LOG_INFO,"url_get info cmd [%d] [%lld] \n",tmpcmd,*outinfo);
    }
    return 0;  
}
static int cmfvpb_read(URLContext *v, uint8_t *buf, int buf_size)
{
    int len, rsize;
    struct cmfvpb *cv;	
    cv = (cmfvpb*)v->priv_data;
    if (!cv || ! cv->input   || !cv->input->prot->url_read) {
        av_log(NULL, AV_LOG_INFO, "cmfvpb_read failed\n");
        return -1;
    }
    if(cv->size == 0)
        rsize = buf_size;
    else
        rsize = FFMIN(buf_size, cv->size -cv->seg_pos);
    //len = ffurl_read(cv->input, buf, buf_size);
RETRY:
    len = ffurl_read(cv->input, buf, rsize);
    if(len == AVERROR(EAGAIN))
        goto RETRY;
    
#if 0
    if(len > 0) {
        FILE *fp = NULL;
        fp = fopen("/temp/cmf_0.ts", "ab+");
        if(fp) {
            fwrite(buf, 1, len, fp);
            fflush(fp);
            fclose(fp);
        }
    }
#endif

    if (len < 0) {
        av_log(NULL, AV_LOG_INFO, "cmfvpb_read failed  len=%d\n",len);
        return -1;
    }
    cv->seg_pos += len;
    return len;
}

static int64_t cmfvpb_seek(URLContext*v, int64_t offset, int whence)
{     
    av_log(NULL, AV_LOG_INFO, "cmfvpb_seek  offset[%lld] whence[%d] \n",offset,whence);
    int64_t lowoffset=0;
    int ret=-1;
    struct cmfvpb *cv;	
    cv = (cmfvpb*)v->priv_data;	
    if (!cv || ! cv->input   || !cv->input->prot->url_seek) {
        av_log(NULL, AV_LOG_INFO, "cmfvpb_seek failed\n");
        return -1;
    }
  av_log(NULL, AV_LOG_INFO, "cmfvpb_seek cv->index [%lld] offset[%lld]  start_offset[%lld] whence[%d] cv->size[%lld]\n", cv->cur_index, offset, cv->start_offset, whence,cv->size);
    if (whence == AVSEEK_SIZE) {
        if (cv->size <= 0) {
            av_log(NULL, AV_LOG_INFO, " cv->size error sieze=%lld\n", cv->size);
        }
        av_log(NULL, AV_LOG_INFO, "cmfvpb_seek cv->size\n",cv->size);
        return cv->size;
    } else if (whence == SEEK_END) {
    		lowoffset=cv->end_offset+offset;
    } else if (whence == SEEK_SET) {
        	lowoffset=cv->start_offset+offset;
    } else if (whence == SEEK_CUR) {
       	lowoffset=cv->start_offset+cv->seg_pos+offset;
    } else if(whence==AVSEEK_SLICE_BYTIME){
            av_log(NULL, AV_LOG_INFO, " AVSEEK_SLICE_BYTIME time [%lld]\n",offset);
            ret = cv->input->prot->url_seek(cv->input,offset, AVSEEK_SLICE_BYTIME);
            if(ret < 0){
                av_log(NULL, AV_LOG_INFO, " AVSEEK_SLICE_BYTIME failed [%x]\n",ret);
	          return -1;
            }
            return 0;
    } else if(whence == AVSEEK_TO_TIME){
            ret = cv->input->prot->url_seek(cv->input,offset, AVSEEK_TO_TIME);
            if(ret < 0){
                av_log(NULL, AV_LOG_INFO, " AVSEEK_TO_TIME failed [%x]\n",ret);
	          return -1;
            }
            return 0;
    } else if(whence == AVSEEK_CMF_TS_TIME){
            ret = cv->input->prot->url_seek(cv->input,offset, AVSEEK_CMF_TS_TIME);
            if(ret < 0){
                av_log(NULL, AV_LOG_INFO, " AVSEEK_CMF_TS_TIME failed [%x]\n",ret);
	          return -1;
            }
            return 0;
    } else{
        av_log(NULL, AV_LOG_INFO, " un support seek cmd else whence  [%x]\n",whence);
	 return -1;
    }
    lowoffset=cv->input->prot->url_seek(cv->input,lowoffset, SEEK_SET);
    if (lowoffset < 0){
            av_log(NULL, AV_LOG_INFO, " cv->input->prot->url_seek failed [%x]\n",lowoffset);
    }
    cv->seg_pos=lowoffset-cv->start_offset;
    return cv->seg_pos;
}

static int cmfvpb_lpread(void *opaque, uint8_t *buf, int buf_size)
{
     struct cmfvpb *cv=opaque;
     if (!cv ) {
        av_log(NULL, AV_LOG_INFO, "cmfvlpread error %s---%d \n",__FUNCTION__,__LINE__);
        return -1;
    }
   return  url_lpread(&cv->vlpcontext,buf,buf_size);
      
}
static int64_t cmfvpb_lpseek(void *opaque, int64_t offset, int whence)
{
    struct cmfvpb *cv=opaque;	
    if (!cv ) {
        av_log(NULL, AV_LOG_INFO, "cmfvpb_lpseek error %s---%d \n",__FUNCTION__,__LINE__);
        return -1;
    }
    return url_lpseek(&cv->vlpcontext, offset, whence);
}
static int64_t cmfvpb_lpexseek(void *opaque, int64_t offset, int whence)
{
    struct cmfvpb *cv=opaque;	
    if (!cv ) {
        av_log(NULL, AV_LOG_INFO, "cmfvpb_lpexseek error %s---%d \n",__FUNCTION__,__LINE__);
        return -1;
    }
    return url_lpexseek(&cv->vlpcontext, offset, whence);
}

int cmfvpb_dup_pb(AVIOContext *pb, struct cmfvpb **cmfvpb, int *index)
{
    struct cmfvpb *cv ;
    AVIOContext *vpb;
    void * read_buffer=NULL;
 
    int ret =-1;
    int lpbufsize=0;
    cv = av_mallocz(sizeof(struct cmfvpb));
    cv->input = pb->opaque;

    cv->vlpcontext.priv_data = cv; 
    cv->vlpcontext.prot = &cv->alcprot;
    cv->vlpcontext.is_slowmedia=1;
    cv->vlpcontext.is_streamed=0;
    cv->vlpcontext.support_time_seek = 0;
    ret= cmfvpb_open(cv, index);
    if (ret < 0){
        av_log(NULL, AV_LOG_INFO, "cmfvpb_open failed %s---%d \n",__FUNCTION__,__LINE__);
        return -1;
    }

    vpb  = av_mallocz(sizeof(AVIOContext));
    if (!vpb) {
        av_free(cv);
        return AVERROR(ENOMEM);
    }
    memcpy(vpb, pb, sizeof(*pb));
    vpb->iscmf=0;
    vpb->is_slowmedia=1;
    vpb->is_streamed=0;
    vpb->support_time_seek = 0;
    read_buffer = av_malloc(INITIAL_BUFFER_SIZE);
    if (!read_buffer) {
        av_free(cv);
        return AVERROR(ENOMEM);
    }

    lpbufsize= (int)cv->size;//the size must be deal with;

    if (lpbufsize >IO_LP_BUFFER_SIZE) {
         lpbufsize= IO_LP_BUFFER_SIZE;
   }  
    av_log(NULL, AV_LOG_INFO, "cmfvpb_LPBUF cv->size[%lld] [%lld]M   lpbuf [%d][%d]M      cv->vlpcontext.is_slowmedia=[%d]\n",cv->size,(cv->size/(1024*1024)),lpbufsize,(lpbufsize/(1024*1024)),cv->vlpcontext.is_slowmedia);
    if(url_lpopen_ex(&cv->vlpcontext,lpbufsize,AVIO_FLAG_READ,cmfvpb_read,cmfvpb_seek)==0){
                ffio_init_context(vpb, read_buffer, INITIAL_BUFFER_SIZE, 0, cv,
                     cmfvpb_lpread, NULL, cmfvpb_lpseek);
	}
    vpb->exseek = cmfvpb_lpexseek;
    vpb->seekable=!vpb->is_streamed;
    cv->pb = vpb;
    *cmfvpb = cv;
    return 0;
}

int cmfvpb_pb_free(struct cmfvpb *cv)
{
    void * read_buffer=NULL;
    if (cv == NULL){
        av_log(NULL,AV_LOG_INFO,"cv = NULL \n");
        return -1;
    }
    if (cv->vlpcontext.lpbuf){
        url_lpfree(&cv->vlpcontext);
    }

    if (cv->pb){
        read_buffer=cv->pb->buffer;
        if (read_buffer) {
            av_free(read_buffer);
        }
        av_free(cv->pb);
    }
    if(cv){
        av_free(cv);
    }
    return 0;
}
