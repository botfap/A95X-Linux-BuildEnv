/*
 * cmfdec io for ffmpeg system
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
 * amlogic:2012.6.29
 */

#include <limits.h>


#include "libavutil/intreadwrite.h"
#include "libavutil/avstring.h"
#include "libavutil/dict.h"
#include "avformat.h"
#include "avio_internal.h"
#include "riff.h"
#include "isom.h"
#include "libavcodec/get_bits.h"
#include <stdio.h>

#if CONFIG_ZLIB
#include <zlib.h>
#endif

#include "qtpalette.h"

#undef NDEBUG
#include <assert.h>

#include "cmfdec.h"
#ifdef DEBUG
#define TRACE()     av_log(NULL, AV_LOG_INFO, "%s---%d \n",__FUNCTION__,__LINE__);
#else
#define TRACE()
#endif

static int cmf_h264_add_header(unsigned char *buf, int size,  AVPacket *pkt)
{
    char nal_start_code[] = {0x0, 0x0, 0x0, 0x1};
    int nalsize;
    unsigned char* p;
    int tmpi;
    unsigned char* extradata = buf;
    int header_len = 0;
    char* buffer = pkt->data;

    p = extradata;
    if ((p[0] == 0 && p[1] == 0 && p[2] == 0 && p[3] == 1) && size < 1024) {
        memcpy(buffer, buf, size);
        pkt->size = size;
        return 0;
    }
    
    if (size < 10) {
        return -1;
    }

    if (*p != 1) {
        return -1;
    }

    int cnt = *(p + 5) & 0x1f; //number of sps
    p += 6;
    for (tmpi = 0; tmpi < cnt; tmpi++) {
        nalsize = (*p << 8) | (*(p + 1));
        memcpy(&(buffer[header_len]), nal_start_code, 4);
        header_len += 4;
        memcpy(&(buffer[header_len]), p + 2, nalsize);
        header_len += nalsize;
        p += (nalsize + 2);
    }

    cnt = *(p++); //Number of pps
    for (tmpi = 0; tmpi < cnt; tmpi++) {
        nalsize = (*p << 8) | (*(p + 1));
        memcpy(&(buffer[header_len]), nal_start_code, 4);
        header_len += 4;
        memcpy(&(buffer[header_len]), p + 2, nalsize);
        header_len += nalsize;
        p += (nalsize + 2);
    }
    if (header_len >= 1024) {
        return -1;
    }
    pkt->size = header_len;
    return 0;
}

static int cmf_probe(AVProbeData *p)
{
    if (p && p->s  && p->s->iscmf) {
	    //return 101;
	    return AVPROBE_SCORE_MAX;
    }
    return 0;
}
static void cmf_reset_packet(AVPacket *pkt)
{
    av_init_packet(pkt);
    pkt->data = NULL;
}

static void cmf_flush_packet_queue(AVFormatContext *s)
{
    AVPacketList *pktl;
    for (;;) {
        pktl = s->packet_buffer;
        if (!pktl) { 
            break;
        }
        s->packet_buffer = pktl->next;
        av_free_packet(&pktl->pkt);
        av_free(pktl);
    }
    while (s->raw_packet_buffer) {
        pktl = s->raw_packet_buffer;
        s->raw_packet_buffer = pktl->next;
        av_free_packet(&pktl->pkt);
        av_free(pktl);
    }
    s->packet_buffer_end =
        s->raw_packet_buffer_end = NULL;
    s->raw_packet_buffer_remaining_size = RAW_PACKET_BUFFER_SIZE;
}
static int cmf_parser_next_slice(AVFormatContext *s, int *index, int first)
{
    int i, j, ret=-1;
    struct cmf *bs = s->priv_data;
    struct cmfvpb *oldvpb = bs->cmfvpb;
    struct cmfvpb *newvpb = NULL;
    AVInputFormat *in_fmt = NULL;
    av_log(s, AV_LOG_INFO, "cmf_parser_next_slice:%d\n", index);
    ret = cmfvpb_dup_pb(s->pb, &newvpb, index);
    if (ret < 0) {
        av_log(s, AV_LOG_INFO, "cmfvpb_dup_pb failed %s---%d [%d]\n",__FUNCTION__,__LINE__,ret);
        return ret;
    }
    if (bs->sctx) {
        avformat_free_context(bs->sctx);
    }
    bs->sctx = NULL;
    if (!(bs->sctx = avformat_alloc_context())) {
        av_log(s, AV_LOG_INFO, "cmf_parser_next_slice avformat_alloc_context failed!\n");
        return AVERROR(ENOMEM);
    }
    ret = av_probe_input_buffer(newvpb->pb, &in_fmt, "", NULL, 0, 0);
    if (ret < 0) {
        av_log(s, AV_LOG_INFO, "av_probe_input_buffer failed %s---%d [%d]\n",__FUNCTION__,__LINE__,ret);
		if (bs->sctx) {
        	avformat_free_context(bs->sctx);
			bs->sctx=NULL;
    	}
		return ret;
    }
    bs->sctx->pb = newvpb->pb;
    ret = avformat_open_input(&bs->sctx, "", NULL, NULL);
    if (ret < 0) {
        av_log(s, AV_LOG_INFO, "cmf_parser_next_slice:avformat_open_input failed \n");
		if (bs->sctx) {
        	avformat_free_context(bs->sctx);
			bs->sctx=NULL;
    	}
        goto fail;
    }
    if (!memcmp(bs->sctx->iformat->name,"flv",3)) {
        ret=av_find_stream_info(bs->sctx);
        av_log(s, AV_LOG_INFO, "xxx-------av_find_stream_info=%d",ret);
    }
    if (first) {
        /* Create new AVStreams for each stream in this chip
             Add into the Best format ;
        */
        if (!memcmp(bs->sctx->iformat->name,"mov",3))
            bs->next_mp4_flag = 1;
        for (j = 0; j < (int)bs->sctx->nb_streams ; j++)   {
            AVStream *st = av_new_stream(s, 0);
            if (!st) {
                ret = AVERROR(ENOMEM);
                goto fail;
            }
            avcodec_copy_context(st->codec, bs->sctx->streams[j]->codec);
        }
        for (i = 0; i < (int)s->nb_streams ; i++)   {
            AVStream *st = s->streams[i];
            AVStream *sst = bs->sctx->streams[i];
            st->id=sst->id;
            if (st->codec->codec_type == CODEC_TYPE_AUDIO) {
                st->time_base.den =  sst->time_base.den;
                st->time_base.num = sst->time_base.num;
                bs->first_slice_audio_index = i;
            }
            if (st->codec->codec_type == CODEC_TYPE_VIDEO) {
                st->time_base.den = sst->time_base.den;
                st->time_base.num = sst->time_base.num;
                bs->first_mp4_video_base_time_num = sst->time_base.num;
                bs->first_mp4_video_base_time_den = sst->time_base.den;
                bs->first_slice_video_index = i;
            }
        }
        s->duration = newvpb->total_duration*1000;
        av_log(s, AV_LOG_INFO, "get duration  [%lld]us [%lld]ms [%lld]s\n", s->duration,newvpb->total_duration,(newvpb->total_duration/1000));
    } else {
        bs->stream_index_changed = 0;
        if (!memcmp(bs->sctx->iformat->name,"mpegts",6)) {
            if(bs->sctx->streams[bs->first_slice_audio_index]->codec->codec_type != s->streams[bs->first_slice_audio_index]->codec->codec_type) {
                bs->stream_index_changed = 1;
                AVStream *temp = NULL;
                AVStream *audio = bs->sctx->streams[bs->first_slice_audio_index];
                temp = bs->sctx->streams[bs->first_slice_video_index];
                bs->sctx->streams[bs->first_slice_video_index] = audio;
                bs->sctx->streams[bs->first_slice_audio_index] = temp;
            }
        }
        if (!memcmp(bs->sctx->iformat->name,"mov",3)) {
            #if 1
            /*
            int audio_index = -1;
            int video_index = -1;
            AVStream *cmf_audio = NULL;
            AVStream *cmf_video = NULL;
            for (i = 0; i < (int)bs->sctx->nb_streams ; i++) {
                if(bs->sctx->streams[i]->codec->codec_type == CODEC_TYPE_AUDIO)
                    cmf_audio = bs->sctx->streams[i];
                if(bs->sctx->streams[i]->codec->codec_type == CODEC_TYPE_VIDEO)
                    cmf_video = bs->sctx->streams[i];
            }
            if(cmf_audio && cmf_video) {*/
                for (i = 0; i < (int)s->nb_streams ; i++) {
                    AVStream *st = s->streams[i];
                    AVStream *sst = bs->sctx->streams[i];
                    if (st->codec->codec_type == CODEC_TYPE_AUDIO) {
                        st->time_base.den =  sst->time_base.den;
                        st->time_base.num = sst->time_base.num;
                        //audio_index = i;
                    }
                    if (st->codec->codec_type == CODEC_TYPE_VIDEO) {
                        st->time_base.den = sst->time_base.den;
                        st->time_base.num = sst->time_base.num;
                        //video_index = i;
                    }
                    if(st->codec->extradata)
                        av_freep(&(st->codec->extradata));
                    if(sst->codec->extradata && sst->codec->extradata_size > 0) {
                        st->codec->extradata = av_malloc(sst->codec->extradata_size + FF_INPUT_BUFFER_PADDING_SIZE);
                        if(st->codec->extradata) {
                             memcpy(st->codec->extradata, sst->codec->extradata, sst->codec->extradata_size);
                             memset(((uint8_t *) st->codec->extradata) + sst->codec->extradata_size, 0, FF_INPUT_BUFFER_PADDING_SIZE);
                        }
                    }
                }
                /*
                if(audio_index >= 0 && bs->sctx->streams[bs->first_mp4_audio_index]->codec->codec_type != s->streams[audio_index]->codec->codec_type) {
                    bs->stream_index_changed = 1;
                    AVStream *temp = NULL;
                    AVStream *audio = s->streams[audio_index];
                    temp = s->streams[video_index];
                    s->streams[video_index] = audio;
                    s->streams[audio_index] = temp;
                    av_log(NULL,AV_LOG_INFO,"--------->tao_cmf_index_change first_mp4_audio_index=%d, audio_index=%d, video_index=%d",bs->first_mp4_audio_index,audio_index,video_index);
                }*/
                bs->next_mp4_flag = 1;
                #endif
            }
        }
    if (memcmp(bs->sctx->iformat->name,"flv",3)) {
        ret = avio_seek(bs->sctx->pb, bs->sctx->media_dataoffset, SEEK_SET);
        if (ret < 0) {
            av_log(s, AV_LOG_INFO, "avio_seek failed %s---%d \n",__FUNCTION__,__LINE__);
            return ret;
        }
    }
    bs->cmfvpb = newvpb;
fail:
    if (oldvpb) {
        cmfvpb_pb_free(oldvpb);
    }
    return ret;
}
static int cmf_read_header(AVFormatContext *s, AVFormatParameters *ap)
{
    struct cmf *bs = s->priv_data;
    memset(bs, 0, sizeof(*bs));
    ap=ap;
    AVIOContext *pb=s->pb;
    if (pb->read_packet || pb->seek) {
        ffio_fdopen_resetlpbuf(pb,0);
        ffio_init_context(pb, pb->buffer, pb->buffer_size, 0, pb->opaque,
                          NULL, NULL, NULL);/*reset old pb*/
        pb->is_slowmedia=0;
        pb->is_streamed=1;
        pb->seekable=0;
        pb->support_time_seek = 0;
    }
    int temp = 0;
    int ret = cmf_parser_next_slice(s, &temp, 1);
    bs->parsering_index = temp;
    return ret;

}

static int cmf_read_packet(AVFormatContext *s, AVPacket *mpkt)
{
    struct cmf *cmf = s->priv_data;
    int ret;
    struct cmfvpb *ci ;
    AVStream *st_parent;
   
retry_read:
    ci = cmf->cmfvpb;
    /*
    int stream_index = -1;
    if(cmf->stream_index_changed == 1 && s->streams[cmf->pkt.stream_index]->codec->codec_type == CODEC_TYPE_VIDEO) {
        stream_index = cmf->first_mp4_audio_index;
    }else if(cmf->stream_index_changed == 1 && s->streams[cmf->pkt.stream_index]->codec->codec_type == CODEC_TYPE_AUDIO) {
        stream_index = cmf->first_mp4_video_index;
    }else{
        stream_index = cmf->pkt.stream_index;
    }*/
	if(cmf->sctx!=NULL) {
           if(cmf->is_cached == 1 && s->streams[cmf->pkt.stream_index]->codec->codec_type == CODEC_TYPE_VIDEO) {
                *mpkt = cmf->cache_pkt;
                cmf->is_cached = 0;
                ret = 0;
           }else{
    	        ret = av_read_frame(cmf->sctx, &cmf->pkt);
           }
           if(ret >= 0 && s->streams[cmf->pkt.stream_index]->codec->codec_type == CODEC_TYPE_VIDEO && cmf->next_mp4_flag == 1) {
                const char * p = cmf->pkt.data;
                if(!p)
                    goto retry_read;
                if(!(p[0] == 0 && p[1] == 0 && p[2] == 0)) {
                    AVPacket header_pkt;
                    av_new_packet(&header_pkt, 1024);
                    int cmf_ret = cmf_h264_add_header(s->streams[cmf->pkt.stream_index]->codec->extradata, s->streams[cmf->pkt.stream_index]->codec->extradata_size, &header_pkt);
                    if(!cmf_ret) {
                        if(!av_dup_packet(&cmf->pkt)) {
                            cmf->cache_pkt = cmf->pkt;
                            cmf->is_cached = 1;
                            *mpkt = header_pkt;
                            cmf->next_mp4_flag = 0;
                            cmf->h264_header_feeding_flag = 1;
                            return 0;
                        }
                    }
                }
           }
#if 0
           if(ret >= 0 && count == 0 &&(s->streams[cmf->pkt.stream_index]->codec->codec_type == CODEC_TYPE_VIDEO) && cmf->next_mp4_flag == 1) {
                FILE * fp = NULL;
                fp = fopen("/temp/cmf_3.dat", "ab+");
                if(fp) {
                    fwrite(cmf->pkt.data, 1, cmf->pkt.size, fp);
                    fflush(fp);
                    fclose(fp);
                    count++;
                }
           }
#endif
	} else {
	    ret = -1;
	}

    if(ret == AVERROR(EAGAIN)) {
        av_log(NULL, AV_LOG_ERROR,"-------->cmf_retry_read");
        goto retry_read;
    }
    if(ret == AVERROR_EXIT)
        return ret;
    if (ret < 0) {
        av_log(NULL,AV_LOG_INFO,"----------->cmf_read_packet ret=%d", ret);
		if(cmf->sctx){
       		cmf_flush_packet_queue(cmf->sctx);
		}
        cmf_reset_packet(&cmf->pkt);
        url_lpreset(&ci->vlpcontext);
        cmf->parsering_index = cmf->parsering_index + 1;
        #if 0
        if(!memcmp(cmf->sctx->iformat->name,"flv",3) && cmf->is_seeked) {
            cmf->parsering_index--;
            cmf->is_seeked = 0;
        }
        #endif
        av_log(s, AV_LOG_INFO, "\n--cmf_read_packet parsernextslice cmf->parsering_index = %lld--\n", cmf->parsering_index);
        if(memcmp(cmf->sctx->iformat->name,"mpegts",6) || ci->hls_stream_type) {
            if (cmf->parsering_index >= ci->total_num){
                av_log(s, AV_LOG_INFO, " cmf_read_packet to lastindex,curindex [%lld] totalnum[%lld]\n", cmf->parsering_index,ci->total_num);
                return AVERROR_EOF;
            }
        }
        ret = cmf_parser_next_slice(s, &(cmf->parsering_index), 0);
        if (ret>=0) {
            av_log(s, AV_LOG_INFO, " goto reread, ret=%d\n", ret);
            goto retry_read;    
        }else{
	    av_log(s, AV_LOG_INFO, "cmf_parser_next_slice failed..., ret=%d\n", ret);
	    return ret;
	}
    }

    st_parent = s->streams[cmf->pkt.stream_index];
    cmf->calc_startpts=av_rescale_rnd(ci->start_time,st_parent->time_base.den,1000*st_parent->time_base.num,AV_ROUND_ZERO);
    if(memcmp(cmf->sctx->iformat->name,"mpegts",6)) {
        if(cmf->h264_header_feeding_flag == 1 && cmf->parsering_index > 0 && s->streams[cmf->pkt.stream_index]->codec->codec_type == CODEC_TYPE_VIDEO) {
            cmf->pkt.pts=(cmf->pkt.pts*cmf->first_mp4_video_base_time_den*st_parent->time_base.num)/(cmf->first_mp4_video_base_time_num*st_parent->time_base.den);
            cmf->calc_startpts=(cmf->calc_startpts*cmf->first_mp4_video_base_time_den*st_parent->time_base.num)/(cmf->first_mp4_video_base_time_num*st_parent->time_base.den);
            //cmf->pkt.pts=av_rescale_rnd(cmf->pkt.pts,st_parent->time_base.den,1000*st_parent->time_base.num,AV_ROUND_ZERO);
        }
        cmf->pkt.pts = cmf->calc_startpts + cmf->pkt.pts;
    }

    if (st_parent->start_time == AV_NOPTS_VALUE) {
        st_parent->start_time  = cmf->pkt.pts;
        av_log(s, AV_LOG_INFO, "first packet st->start_time [0x%llx] [0x%llx]\n", st_parent->start_time, cmf->pkt.stream_index);
    }
    *mpkt = cmf->pkt;

    if(s->streams[cmf->pkt.stream_index]->codec->codec_type == CODEC_TYPE_AUDIO && (!memcmp(cmf->sctx->iformat->name,"mpegts",6)||!memcmp(cmf->sctx->iformat->name,"mpegps",6))) {
        mpkt->flags|=AV_PKT_FLAG_AAC_WITH_ADTS_HEADER;
    }
    return 0;
}
static
int cmf_read_seek(AVFormatContext *s, int stream_index, int64_t sample_time, int flags)
{
    av_log(NULL, AV_LOG_INFO, "\n------- cmf read_seek:BEGAN stream_index=%d ,sample_time=%lld,flags=%d --------\n\n", stream_index, sample_time, flags);
    struct cmf *cmf = s->priv_data;
    struct cmfvpb *ci = cmf->cmfvpb ;
    int ret;
    int64_t remain_seektime;
    int64_t seektimestamp;
    AVStream *st;
    if(cmf->sctx == NULL||cmf->sctx->iformat == NULL){
        av_log(NULL,AV_LOG_ERROR,"cmf sctx[%s] or iformat is NULL\n",cmf->sctx == NULL?"null":"valid");
        return -1;
    }
    av_log(NULL, AV_LOG_INFO, "cmf read_seek:TOTALINFO  index[%lld] total_num [%lld] total_duration [%lld] \n",ci->cur_index,ci->total_num, ci->total_duration);
    av_log(NULL, AV_LOG_INFO, "cmf read_seek:SIZEINFO    index[%lld] start_offset [0x%llx] end_offset [0x%llx] slice_size[0x%llx] \n", ci->cur_index, ci->start_offset, ci->end_offset, ci->size);
    av_log(NULL, AV_LOG_INFO, "cmf read_seek:TIMEINFO    index[%lld] start_time [%lld]ms end_time [%lld]ms curslice_duration[%lld]ms \n", ci->cur_index, ci->start_time, ci->end_time, ci->curslice_duration);
    
     if(stream_index < 0){
        stream_index= av_find_default_stream_index(s);
        if(stream_index < 0)
            return -1;
    }

    st= s->streams[stream_index];
    
    seektimestamp=av_rescale_rnd(sample_time,st->time_base.num*1000,st->time_base.den,AV_ROUND_ZERO);//pts->ms
    
    av_log(s, AV_LOG_INFO, "cmf read_seek: checkin pts [%lld] seektimestamp[%lld]ms streamindex [%d]\n",sample_time,seektimestamp,stream_index);
    if (!memcmp(cmf->sctx->iformat->name,"mpegts",6)) {
        flags = AVSEEK_CMF_TS_TIME;
    }
              
    if ((seektimestamp >= ci->start_time)&&(seektimestamp <= ci->end_time )){
        //in cur slice;
        av_log(s, AV_LOG_INFO, "cmf read_seek:IN CUR SLICE realtime = [%lld]ms  start-end [%lld-%lld] calctoend[%lld]ms\n",seektimestamp,ci->start_time,ci->end_time,ci->end_time-seektimestamp);
        seektimestamp = seektimestamp - ci->start_time;
        seektimestamp = seektimestamp*1000;//ms->us
        av_log(NULL, AV_LOG_INFO, "cmf read_seek:index[%lld] curduration[%lld] seektimestamp[%lld]us\n", ci->cur_index, ci->curslice_duration,seektimestamp);
        int seek_unit = 1*1000*1000;
        while((ret = av_seek_frame(cmf->sctx, stream_index, seektimestamp, flags)) < 0){
            seektimestamp += seek_unit * (-1);
            if(seektimestamp < 0)
                seek_unit *= -1;
            if(seektimestamp/1000 + ci->start_time > ci->end_time)
                break;
        }
        if (ret < 0) {
            av_log(s, AV_LOG_INFO, "cmf read_seek:av_seek_frame Failed in cur slice\n");
        }
    }else if ((seektimestamp < ci->start_time)||(seektimestamp > ci->end_time )){
        av_log(s, AV_LOG_INFO, "cmf read_seek:OUT_CUR_SLICE firstcheck seektimestamp[%lld]ms  start-end [%lld---%lld]ms\n",seektimestamp,ci->start_time,ci->end_time);
        //out of cur slice and in the total slice;
        //seek to cur slice;

        ret = url_fseekslicebytime(cmf->cmfvpb->pb,seektimestamp, AVSEEK_SLICE_BYTIME);
        if (ret < 0){
            av_log(s, AV_LOG_INFO, "cmf read_seek: seekbytime error seektimestamp[%lld]ms\n",seektimestamp);
            return -1;
        }
        cmf->is_seeked = 1;
        cmfvpb_getinfo(ci,AVCMD_SLICE_INDEX,0,&ci->cur_index);
        cmfvpb_getinfo(ci, AVCMD_SLICE_START_OFFSET, 0, &ci->start_offset); 
        //cmfvpb_getinfo(ci, AVCMD_SLICE_SIZE, 0 , &ci->size);
        cmfvpb_getinfo(ci, AVCMD_SLICE_STARTTIME, 0 , &ci->start_time);
        cmfvpb_getinfo(ci, AVCMD_SLICE_ENDTIME, 0 , &ci->end_time);

        //ci->end_offset = ci->start_offset + ci->size;
        ci->curslice_duration=ci->end_time-ci->start_time;
        
        cmf->parsering_index = ci->cur_index;
        remain_seektime = seektimestamp - ci->start_time;
        cmf_flush_packet_queue(cmf->sctx);
        cmf_reset_packet(&cmf->pkt);
        ret = cmf_parser_next_slice(s, &(cmf->parsering_index), 0);
        cmfvpb_getinfo(ci, AVCMD_SLICE_SIZE, 0 , &ci->size);
        ci->end_offset = ci->start_offset + ci->size;
        av_log(s, AV_LOG_INFO, "cmf read_seek:OUT_CUR_SLICE switchtoincurindex index[%lld] seektimestamp[%lld]ms start-end [%lld--%lld]ms  remainseektime[%lld]ms\n",ci->cur_index,seektimestamp,ci->start_time,ci->end_time,remain_seektime);

        if (ret>=0) {
             av_log(s, AV_LOG_INFO, "cmf read_seek:switchto incurindex index[%lld] seektimestamp[%lld]ms starttime[%lld]ms endtime[%lld]ms remainseektime[%lld]ms\n",ci->cur_index,seektimestamp,ci->start_time,ci->end_time,remain_seektime);
            //parser succeed,seek in cur slice;
             remain_seektime=remain_seektime*1000;//ms->us;
             av_log(s, AV_LOG_INFO, "cmf read_seek:switchto incurindex:  index[%lld] remain_seektime[%lld]ms  start-end [%lld - %lld]ms calctoend[%lld]\n",ci->cur_index,remain_seektime/1000,ci->start_time,ci->end_time,(ci->end_time-seektimestamp));
             //seek to remainseektime;
             int seek_unit = 1*1000*1000;
             while((ret = av_seek_frame(cmf->sctx, stream_index, remain_seektime, flags)) < 0){
                remain_seektime += seek_unit * (-1);
                if(remain_seektime < 0)
                    seek_unit *= -1;
                if(remain_seektime/1000 + ci->start_time > ci->end_time)
                    break;
            }
            if (ret < 0) {
                av_log(s, AV_LOG_INFO, "cmf read_seek:out_curslice av_seek_frame Failed out of cur slice , cur_slice index [%lld]\n",ci->cur_index);
                return -1;
            }
        }else{
             av_log(s, AV_LOG_INFO, "cmf parser_next_slice failed %s---%d  [%d]\n",__FUNCTION__,__LINE__,ret);
             return -1;
        }
    }else if (seektimestamp > s->duration/1000){
          av_log(s, AV_LOG_INFO, "cmf read_seek CMFSEEKERROR out of the total slice [%lld] [%lld]\n",seektimestamp,s->duration/1000);
          return -1;
        //out of total slice;
    }else {
        av_log(NULL, AV_LOG_INFO, "cmf read_seek CMFSEEKERROR index[%lld] start_time [%lld] end_time [%lld] curduration[%lld] seektimestamp[%lld]us\n", ci->cur_index, ci->start_time, ci->end_time, ci->curslice_duration,seektimestamp);
        return -1;
    }
    av_log(NULL, AV_LOG_INFO, "\n------cmf read_seek:END    index[%lld] start_time [%lld]ms end_time [%lld]ms curslice_duration[%lld]ms -------\n\n", ci->cur_index, ci->start_time, ci->end_time, ci->curslice_duration);
    return 0;
}
static
int cmf_read_close(AVFormatContext *s)
{
    struct cmf *bs = s->priv_data;
    if (bs && bs->sctx) {
        avformat_free_context(bs->sctx);
    }	
     cmfvpb_pb_free(bs->cmfvpb);
    return 0;
}

AVInputFormat ff_cmf_demuxer = {
    "cmf",
    NULL_IF_CONFIG_SMALL("cmf demux"),
    sizeof(struct cmf),
    cmf_probe,
    cmf_read_header,
    cmf_read_packet,
    cmf_read_close,
    cmf_read_seek,
    .flags=AVFMT_NOGENSEARCH,
};

