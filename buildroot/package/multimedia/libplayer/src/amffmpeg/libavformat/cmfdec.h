#ifndef AVFORMAT_CMFDEMUX_H
#define AVFORMAT_CMFDEMUX_H

#include "avio.h"
#include "internal.h"
#include "cmfvpb.h"




typedef struct cmf {
    struct cmfvpb *cmfvpb;
    AVFormatContext *sctx;
    AVPacket pkt;
    AVPacket cache_pkt;
    int is_seeked;
    int next_mp4_flag;
    int stream_index_changed;
    int h264_header_feeding_flag;
    int is_cached;
    int first_slice_video_index;
    int first_slice_audio_index;
    int first_mp4_video_base_time_num;
    int first_mp4_video_base_time_den;
    int64_t parsering_index;
    int64_t calc_startpts;
} cmf_t;




#endif






