#include <stdio.h>
#include <stdlib.h>
#include <player_priv.h>
#include <log_print.h>
#include "thumbnail_type.h"

static inline void calc_aspect_ratio(rational *ratio, struct stream *stream)
{
    int num, den;

    av_reduce(&num, &den,
              stream->pCodecCtx->width * stream->pCodecCtx->sample_aspect_ratio.num,
              stream->pCodecCtx->height * stream->pCodecCtx->sample_aspect_ratio.den,
              1024 * 1024);
    ratio->num = num;
    ratio->den = den;
}
static int av_read_next_video_frame(AVFormatContext *pFormatCtx, AVPacket *pkt,int vindex)
{
	int r=-1;
	int retry=500;
	do{
		r = av_read_frame(pFormatCtx,pkt);
		if(r==0 && pkt->stream_index==vindex){		       
			break;	
		}
		av_free_packet(pkt);
		if(r<0)
			break;
		else
			r=-2;/*a audio or other frame.*/
	}while(retry-->0);
	return r;
}
static void find_best_keyframe(AVFormatContext *pFormatCtx, int video_index, int count, int64_t *time, int64_t *offset, int *maxsize)
{
    int i = 0;
    int maxFrameSize = 0;
    int64_t thumbTime = 0;
    int64_t thumbOffset = 0;
    AVPacket packet;
    int r = 0;
    int find_ok = 0;
    int keyframe_index=0;
    AVStream *st=pFormatCtx->streams[video_index];
	int havepts=0;
	int nopts=0;
	*maxsize=0;
    if(count<=0){
	   float newcnt=-0.1;	
	   count=100;
	   if(am_getconfig_float("libplayer.thumbnail.scan.count",&newcnt)>=0 && newcnt>=1)
	   	count=(int)newcnt;
    }
	
    do{
	   r = av_read_next_video_frame(pFormatCtx, &packet,video_index);
	   if(r<0)
	   	break;
          log_debug("[find_best_keyframe][%d]read frame packet.size=%d,pts=%lld\n",i,packet.size,packet.pts);
  	   havepts=(packet.pts>0||havepts);
  	   nopts=(i>10)&&!havepts;
  	   if (packet.size > maxFrameSize && (packet.pts>=0 || nopts)) { //packet.pts>=0 can used for seek.
		maxFrameSize = packet.size;
		thumbTime = packet.pts;
		thumbOffset = avio_tell(pFormatCtx->pb) - packet.size;
		keyframe_index=i;
		find_ok=1;
	   }        	   
	   av_free_packet(&packet);
	   if(i>5 && find_ok && maxFrameSize>100*1024/(1+i/20)) 
	   	 break;
    }while(i++<count);

    if(find_ok){
	 log_debug("[%s]return thumbTime=%lld thumbOffset=%llx\n", __FUNCTION__, thumbTime, thumbOffset);
	 	if(thumbTime>=0&&thumbTime != AV_NOPTS_VALUE)
    		*time = av_rescale_q(thumbTime, st->time_base, AV_TIME_BASE_Q);
		else
			*time = AV_NOPTS_VALUE;
    	*offset = thumbOffset;
		*maxsize = maxFrameSize;
	r=0;
    }else{
    	log_print("[%s]find_best_keyframe failed\n", __FUNCTION__);		
    }
    return r;	
}

static void find_thumbnail_frame(AVFormatContext *pFormatCtx, int video_index, int64_t *thumb_time, int64_t *thumb_offset,int *pmaxframesize)
{
    int64_t thumbTime = 0;
    int64_t thumbOffset = 0;
    AVPacket packet;
    AVStream *st = pFormatCtx->streams[video_index];
    int duration = pFormatCtx->duration / AV_TIME_BASE;
 ///   int64_t init_seek_time = (duration > 0) ? MIN(10, duration >> 1) : 10;
    int64_t init_seek_time = 10;	
    int ret = 0;
    float starttime;
	int maxframesize;
    if(am_getconfig_float("libplayer.thumbnail.starttime",&starttime)>=0 && starttime>=0)
    {
	   	init_seek_time=(int64_t)starttime;
		if(init_seek_time >= duration)
			init_seek_time=duration-1;
		if(init_seek_time<=0)
			init_seek_time=0;
    }else{
    	if(duration > 360)
			init_seek_time=120;/*long file.don't do more seek..*/
		else if(duration>6){
			init_seek_time=duration/3;
		}else{
			init_seek_time=0;/*file is too short,try from start.*/
		}
    }
    init_seek_time *= AV_TIME_BASE;
    log_debug("[find_thumbnail_frame]duration=%lld init_seek_time=%lld\n", pFormatCtx->duration, init_seek_time);
    //init_seek_time = av_rescale_q(init_seek_time, st->time_base, AV_TIME_BASE_Q);
    //log_debug("[find_thumbnail_frame]init_seek_time=%lld timebase=%d:%d video_index=%d\n",init_seek_time,st->time_base.num,st->time_base.den, video_index);
    ret = av_seek_frame(pFormatCtx, video_index,init_seek_time, AVSEEK_FLAG_BACKWARD);
    if (ret < 0) {
        avio_seek(pFormatCtx->pb, 0, SEEK_SET);
        log_error("[%s]seek error, reset offset to 0\n", __FUNCTION__);
        return;
    }
    log_debug("[find_thumbnail_frame]offset=%llx \n", avio_tell(pFormatCtx->pb));

    find_best_keyframe(pFormatCtx, video_index, 0, &thumbTime, &thumbOffset,&maxframesize);

    if (thumbTime != AV_NOPTS_VALUE) {
        *thumb_time = thumbTime;
    }else{
    	*thumb_time = AV_NOPTS_VALUE;
    }
    *thumb_offset = thumbOffset;
	*pmaxframesize=maxframesize;
    log_debug("[find_thumbnail_frame]return thumb_time=%lld thumb_offset=%lld\n", *thumb_time, *thumb_offset);
}

void * thumbnail_res_alloc(void)
{
    struct video_frame * frame;

    frame = (struct video_frame *)malloc(sizeof(struct video_frame));
    if (frame == NULL) {
        return NULL;
    }
    memset(frame, 0, sizeof(struct video_frame));
    frame->stream.videoStream=-1;
    av_register_all();

    return (void *)frame;
}

//#define DUMP_INDEX
int thumbnail_find_stream_info(void *handle, const char* filename)
{
    struct video_frame *frame = (struct video_frame *)handle;
    struct stream *stream = &frame->stream;
    int i;
    if (av_open_input_file(&stream->pFormatCtx, filename, NULL, 0, NULL) != 0) {
        log_print("Coundn't open file %s !\n", filename);
        goto err;
    }
    for (i = 0; i < stream->pFormatCtx->nb_streams; i++) {        
        if (stream->pFormatCtx->streams[i]->codec->codec_type == CODEC_TYPE_VIDEO) {
                      stream->videoStream = i;
            break;
        }
    }
    //thumbnail just need the info of stream, so do not need fast_switch
    stream->pFormatCtx->pb->local_playback = 1;
    
    if (av_find_stream_info(stream->pFormatCtx) < 0) {
        log_print("Coundn't find stream information !\n");
        goto err1;
    }
#ifdef DUMP_INDEX
    int i, j;
    AVStream *pStream;
    log_print("*********************************************\n");
    for (i = 0; i < stream->pFormatCtx->nb_streams; i ++) {
        pStream = stream->pFormatCtx->streams[i];
        if (pStream) {
            for (j = 0; j < pStream->nb_index_entries; j++) {
                log_print("stream[%d]:idx[%d] pos:%llx time:%llx\n", i, j, pStream->index_entries[j].pos, pStream->index_entries[j].timestamp);
            }
        }
    }
    log_print("*********************************************\n");
#endif

    return 0;

err1:
    av_close_input_file(stream->pFormatCtx);
err:
    memset(&frame->stream, 0, sizeof(struct stream));

    return -1;
}

int thumbnail_find_stream_info_end(void *handle)
{
    struct video_frame *frame = (struct video_frame *)handle;
    struct stream *stream = &frame->stream;

    av_close_input_file(stream->pFormatCtx);

    return 0;
}

int thumbnail_decoder_open(void *handle, const char* filename)
{
    int i;
    int video_index = -1;
    struct video_frame *frame = (struct video_frame *)handle;
    struct stream *stream = &frame->stream;

    log_debug("thumbnail open file:%s\n", filename);
	update_loglevel_setting();
    if (av_open_input_file(&stream->pFormatCtx, filename, NULL, 0, NULL) != 0) {
        log_print("Coundn't open file %s !\n", filename);
        goto err;
    }

    //thumbnail extract frame, so need fast_switch
    stream->pFormatCtx->pb->local_playback = 1;

    if (av_find_stream_info(stream->pFormatCtx) < 0) {
        log_print("Coundn't find stream information !\n");
        goto err1;
    }

    //dump_format(stream->pFormatCtx, 0, filename, 0);
    for (i = 0; i < stream->pFormatCtx->nb_streams; i++) {
        if (stream->pFormatCtx->streams[i]->codec->codec_type == CODEC_TYPE_VIDEO) {
            video_index = i;
            break;
        }
    }

    if (video_index == -1) {
        log_print("Didn't find a video stream!\n");
        goto err1;
    }

    find_thumbnail_frame(stream->pFormatCtx, video_index, &frame->thumbNailTime, &frame->thumbNailOffset,&frame->maxframesize);

    stream->videoStream = video_index;
    stream->pCodecCtx = stream->pFormatCtx->streams[video_index]->codec;
    if (stream->pCodecCtx == NULL) {
        log_print("pCodecCtx is NULL !\n");
    }

    frame->width = stream->pCodecCtx->width;
    frame->height = stream->pCodecCtx->height;

    stream->pCodec = avcodec_find_decoder(stream->pCodecCtx->codec_id);
    if (stream->pCodec == NULL) {
        log_print("Didn't find codec!\n");
        goto err1;
    }

    if (avcodec_open(stream->pCodecCtx, stream->pCodec) < 0) {
        log_print("Couldn't open codec!\n");
        goto err1;
    }

    frame->duration = stream->pFormatCtx->duration;

    stream->pFrameYUV = avcodec_alloc_frame();
    if (stream->pFrameYUV == NULL) {
        log_print("alloc YUV frame failed!\n");
        goto err2;
    }

    stream->pFrameRGB = avcodec_alloc_frame();
    if (stream->pFrameRGB == NULL) {
        log_print("alloc RGB frame failed!\n");
        goto err3;
    }

    frame->DataSize = avpicture_get_size(DEST_FMT, frame->width, frame->height);
    frame->data = (char *)malloc(frame->DataSize);
    if (frame->data == NULL) {
        log_print("alloc buffer failed!\n");
        goto err4;
    }

    avpicture_fill((AVPicture *)stream->pFrameRGB, frame->data, DEST_FMT, frame->width, frame->height);

    return 0;

err4:
    av_free(stream->pFrameRGB);
err3:
    av_free(stream->pFrameYUV);
err2:
    avcodec_close(stream->pCodecCtx);
err1:
    av_close_input_file(stream->pFormatCtx);
err:
    memset(&frame->stream, 0, sizeof(struct stream));

    return -1;
}

#define TRY_DECODE_MAX (50)
#define READ_FRAME_MAX (10*25)
#define READ_FRAME_MIN (2*25)

int thumbnail_extract_video_frame(void *handle, int64_t time, int flag)
{
    int frameFinished = 0;  
    int tryNum = 0;
    int i = 0;
    int64_t ret;

    struct video_frame *frame = (struct video_frame *)handle;
    struct stream *stream = &frame->stream;
    AVFormatContext *pFormatCtx = stream->pFormatCtx;
    AVPacket        packet;
    AVCodecContext *pCodecCtx = pFormatCtx->streams[stream->videoStream]->codec;

    if (time >= 0) {
        if (av_seek_frame(pFormatCtx, stream->videoStream, time, AVSEEK_FLAG_BACKWARD) < 0) {
            log_error("[thumbnail_extract_video_frame]av_seek_frame failed!");
        }
        find_best_keyframe(pFormatCtx, stream->videoStream, 0, &frame->thumbNailTime, &frame->thumbNailOffset,&frame->maxframesize);
        log_debug("[thumbnail_extract_video_frame:%d]time=%lld time=%lld  offset=%lld!ret=%d\n", 
                __LINE__,time, frame->thumbNailTime, frame->thumbNailOffset, ret);
    }

    if (frame->thumbNailTime>=0 && frame->thumbNailTime != AV_NOPTS_VALUE) {
        log_debug("seek to thumbnail frame by timestamp(%lld)!curoffset=%llx\n", frame->thumbNailTime, avio_tell(pFormatCtx->pb));
        if (av_seek_frame(pFormatCtx, stream->videoStream, frame->thumbNailTime, AVSEEK_FLAG_BACKWARD) < 0) {
            avio_seek(pFormatCtx->pb, frame->thumbNailOffset, 0);
            log_error("[thumbnail_extract_video_frame]av_seek_frame failed!seek to thumbNailOffset %x\n",frame->thumbNailOffset);
        }
        log_debug("after seek by time, offset=%llx!\n", avio_tell(pFormatCtx->pb));
    } else {
        log_debug("seek to thumbnail frame by offset(%lld)!\n", frame->thumbNailOffset);
        ret = avio_seek(pFormatCtx->pb, frame->thumbNailOffset, SEEK_SET);
        log_debug("after seek by offset, offset=%llx!ret=%llx\n", avio_tell(pFormatCtx->pb), ret);
    }

    avcodec_flush_buffers(stream->pCodecCtx);
	i=0;
    while (av_read_next_video_frame(pFormatCtx, &packet,stream->videoStream) >= 0) {
	     AVFrame         *pFrame=NULL;
	     log_debug("[%s] av_read_frame frame size=%d,pts=%lld\n", __FUNCTION__, packet.size,packet.pts);
		 i++;
	     if(packet.size<MAX(frame->maxframesize/10,packet.size) && i < READ_FRAME_MIN){
		 	continue;/*skip small size packets,it maybe a black frame*/
	     }
            avcodec_decode_video2(stream->pCodecCtx, stream->pFrameYUV, &frameFinished, &packet);
	     pFrame=stream->pFrameYUV	;
            log_debug("[%s]decode video frame, finish=%d key=%d offset=%llx type=%dcodec_id=%x,quality=%d tryNum=%d\n", __FUNCTION__,
                      frameFinished, pFrame->key_frame, avio_tell(pFormatCtx->pb), pFrame->pict_type, pCodecCtx->codec_id,pFrame->quality,tryNum);
         if (frameFinished && 
		 ((pFrame->key_frame && pFrame->pict_type == AV_PICTURE_TYPE_I) ||
		 (pFrame->key_frame && pFrame->pict_type == AV_PICTURE_TYPE_SI) ||
		 (pFrame->key_frame && pFrame->pict_type == AV_PICTURE_TYPE_BI) ||
		 (tryNum>4 && pFrame->key_frame) || 
		 (tryNum>5 && pFrame->pict_type == AV_PICTURE_TYPE_I) ||
		 (tryNum>6 && pFrame->pict_type == AV_PICTURE_TYPE_SI) ||
		 (tryNum>7 && pFrame->pict_type == AV_PICTURE_TYPE_BI) ||
		 (tryNum>(TRY_DECODE_MAX-1) && i > READ_FRAME_MAX && pFrame->pict_type == AV_PICTURE_TYPE_P) || 
		 (tryNum>(TRY_DECODE_MAX-1) && i > READ_FRAME_MAX && pFrame->pict_type == AV_PICTURE_TYPE_B) || 
		 (tryNum>(TRY_DECODE_MAX-1) && i > READ_FRAME_MAX && pFrame->pict_type == AV_PICTURE_TYPE_S) || 
		 (0))) /*not find a I FRAME too long,try normal frame*/
          {                
                log_debug("[%s]pCodecCtx->codec_id=%x tryNum=%d\n", __FUNCTION__, pCodecCtx->codec_id, tryNum);

                struct SwsContext *img_convert_ctx;
                img_convert_ctx = sws_getContext(stream->pCodecCtx->width, stream->pCodecCtx->height,
                                                 stream->pCodecCtx->pix_fmt,
                                                 frame->width, frame->height, DEST_FMT, SWS_BICUBIC,
                                                 NULL, NULL, NULL);
                if (img_convert_ctx == NULL) {
                    log_print("can not initialize the coversion context!\n");
                    av_free_packet(&packet);
                    break;
                }

                sws_scale(img_convert_ctx, stream->pFrameYUV->data, stream->pFrameYUV->linesize, 0,
                          frame->height, stream->pFrameRGB->data, stream->pFrameRGB->linesize);
                av_free_packet(&packet);
                goto ret;
        }
        av_free_packet(&packet);
	 if(tryNum++>TRY_DECODE_MAX && i > READ_FRAME_MAX)
	 	break;
	 if(tryNum%10==0)
        usleep(100);
    }

    if (frame->data) {
        free(frame->data);
    }
    av_free(stream->pFrameRGB);
    av_free(stream->pFrameYUV);
    avcodec_close(stream->pCodecCtx);
    av_close_input_file(stream->pFormatCtx);
    memset(&frame->stream, 0, sizeof(struct stream));
    return -1;

ret:
    return 0;
}

int thumbnail_read_frame(void *handle, char* buffer)
{
    int i;
    int index = 0;
    struct video_frame *frame = (struct video_frame *)handle;
    struct stream *stream = &frame->stream;

    for (i = 0; i < frame->height; i++) {
        memcpy(buffer + index, stream->pFrameRGB->data[0] + i * stream->pFrameRGB->linesize[0], frame->width * 2);
        index += frame->width * 2;
    }

    return 0;
}

void thumbnail_get_video_size(void *handle, int* width, int* height)
{
    struct video_frame *frame = (struct video_frame *)handle;

    *width = frame->width;
    *height = frame->height;
}

float thumbnail_get_aspect_ratio(void *handle)
{
    struct video_frame *frame = (struct video_frame *)handle;
    struct stream *stream = &frame->stream;

    calc_aspect_ratio(&frame->displayAspectRatio, stream);

    if (!frame->displayAspectRatio.num || !frame->displayAspectRatio.den) {
        return (float)frame->width / frame->height;
    } else {
        return (float)frame->displayAspectRatio.num / frame->displayAspectRatio.den;
    }
}

void thumbnail_get_duration(void *handle, int64_t *duration)
{
    struct video_frame *frame = (struct video_frame *)handle;
    struct stream *stream = &frame->stream;

    *duration = stream->pFormatCtx->duration;
}

int thumbnail_get_key_metadata(void* handle, char* key, const char** value)
{
    struct video_frame *frame = (struct video_frame *)handle;
    struct stream *stream = &frame->stream;
    AVDictionaryEntry *tag = NULL;

    if (!stream->pFormatCtx->metadata) {
        return 0;
    }

    if(!memcmp(key, "rotate", 6)) {
            int rot;
            if(frame->stream.videoStream==-1){/*no video.*/
                return 0;
            }
            thumbnail_get_video_rotation(handle,&rot);
            char *tmp = (char*)av_malloc(32);
            memset(tmp, 0x00, 32);
            sprintf(tmp, "%d", rot);
            *value = tmp;
            return 1;
    } 
    tag = av_dict_get(stream->pFormatCtx->metadata, key, tag, 0);
    if (tag) {
        *value = tag->value;
        return 1;
    }

    return 0;
}

int thumbnail_get_key_data(void* handle, char* key, const void** data, int* data_size)
{
    struct video_frame *frame = (struct video_frame *)handle;
    struct stream *stream = &frame->stream;
    AVDictionaryEntry *tag = NULL;

    if (!stream->pFormatCtx->metadata) {
        return 0;
    }

    if (av_dict_get(stream->pFormatCtx->metadata, key, tag, AV_METADATA_IGNORE_SUFFIX)) {
        *data = stream->pFormatCtx->cover_data;
        *data_size = stream->pFormatCtx->cover_data_len;
        return 1;
    }

    return 0;
}
int thumbnail_get_tracks_info(void *handle, int *vtracks, int *atracks, int *stracks)
{
    struct video_frame *frame = (struct video_frame *)handle;
    struct stream *stream = &frame->stream;
    AVStream **avs = stream->pFormatCtx->streams;
    int i;
    *vtracks = 0;
    *atracks = 0;
    *stracks = 0;
    for (i = 0; i < stream->pFormatCtx->nb_streams; i++) {
        if (avs[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            (*vtracks)++;
        } else if (avs[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
            (*atracks)++;
        } else if (avs[i]->codec->codec_type == AVMEDIA_TYPE_SUBTITLE) {
            (*stracks)++;
        } else
            ;
    }
    log_debug("thumbnail_get_tracks_info v:%d a:%d s:%d \n", *vtracks, *atracks, *stracks);
    return 0;
}
void thumbnail_get_video_rotation(void *handle, int* rotation)
{
    struct video_frame *frame = (struct video_frame *)handle;
    struct stream *stream = &frame->stream;
    int stream_rotation = stream->pFormatCtx->streams[stream->videoStream]->rotation_degree;

    switch (stream_rotation) {
    case 1:
        *rotation = 90;
        break;

    case 2:
        *rotation = 180;
        break;

    case 3:
        *rotation = 270;
        break;

    default:
        *rotation = 0;
        break;
    }

    return;
}

int thumbnail_decoder_close(void *handle)
{
    struct video_frame *frame = (struct video_frame *)handle;
    struct stream *stream = &frame->stream;

    if (frame->data) {
        free(frame->data);
    }
    if (stream->pFrameRGB) {
        av_free(stream->pFrameRGB);
    }
    if (stream->pFrameYUV) {
        av_free(stream->pFrameYUV);
    }

    avcodec_close(stream->pCodecCtx);
    av_close_input_file(stream->pFormatCtx);

    return 0;
}

void thumbnail_res_free(void* handle)
{
    struct video_frame *frame = (struct video_frame *)handle;

    if (frame) {
        free(frame);
    }
}
