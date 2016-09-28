#ifndef AVFORMAT_CMFDEMUXVPB_H
#define AVFORMAT_CMFDEMUXVPB_H

#include "avio.h"
#include "internal.h"
#include "dv.h"

typedef struct cmfvpb {
    uint8_t* read_buffer;
    int reading_flag;
    URLContext *input;
    URLContext vlpcontext;
    URLProtocol alcprot;
    AVIOContext *pb;
    int hls_stream_type;
    int64_t start_offset;
    int64_t end_offset;
    int64_t start_time;
    int64_t end_time;
    int64_t curslice_duration;
    int64_t cur_index;
    int64_t size;
    int64_t total_duration;
    int64_t total_num;
    int64_t seg_pos;
} cmfvpb;

#define INITIAL_BUFFER_SIZE 32768
int cmfvpb_dup_pb(AVIOContext *pb, struct cmfvpb **cmfvpb, int  *index);
int cmfvpb_pb_free(struct cmfvpb *ci);
int cmfvpb_getinfo(struct cmfvpb *cv, int cmd,int flag,int64_t*info);


#endif
