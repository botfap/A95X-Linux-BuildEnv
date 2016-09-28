//coded by peter,20130424

#define LOG_NDEBUG 0
#define LOG_TAG "HlsCmf"

#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>

#include "hls_m3uparser.h"
#include "hls_m3ulivesession.h"
#ifdef HAVE_ANDROID_OS
#include "hls_common.h"
#else
#include "hls_debug.h"
#endif

#include "hls_cmf_impl.h"


int hls_cmf_get_clip_num(void* session){
    return m3u_session_get_segment_num(session);
}
int64_t hls_cmf_seek_by_size(void* session,CmfPrivContext_t* ctx,int64_t pos){
    int64_t ret = m3u_session_hybrid_seek(session,ctx->cur_clip_st,pos,ctx->interrupt_func_cb);
    return ret;
}
int hls_cmf_shift_index_by_time(void* session,CmfPrivContext_t* ctx,int64_t posUs){
    M3uBaseNode* node =  m3u_session_get_index_by_timeUs(session,posUs);
   
    if(node==NULL){
        LOGE("failed to get segment info by time :%lld us\n",posUs);
        return -1;
    }
    ctx->cur_clip_index = node->index;
    ctx->cur_clip_st = node->startUs;
    ctx->cur_clip_end = node->startUs+node->durationUs;
    ctx->cur_clip_offset = node->range_offset;
    ctx->cur_clip_len = node->range_length;
    strcpy(ctx->cur_clip_path,node->fileUrl);
    return node->index;
}
int64_t hls_cmf_seek_by_index(void* session,CmfPrivContext_t* ctx,int index){
    M3uBaseNode* node = NULL;

    node = m3u_session_seek_by_index(session,ctx->cur_clip_index ,index,ctx->interrupt_func_cb); 
   
    if(node==NULL){
        LOGE("failed to get segment info by index:%d\n",index);
        return -1;
    }
    ctx->cur_clip_index = index;
    ctx->cur_clip_st = node->startUs;
    ctx->cur_clip_end = node->startUs+node->durationUs;
    ctx->cur_clip_offset = node->range_offset;
    ctx->cur_clip_len = node->range_length;
    strcpy(ctx->cur_clip_path,node->fileUrl);
    if(ctx->interrupt_func_cb!=NULL&&ctx->interrupt_func_cb()>0){
        return -1;
    }
    return node->startUs;
}
int hls_cmf_get_fsize(void* session,CmfPrivContext_t* ctx,int type){
    return m3u_session_get_segment_size(session,ctx->cur_clip_path,ctx->cur_clip_index,type);
}

