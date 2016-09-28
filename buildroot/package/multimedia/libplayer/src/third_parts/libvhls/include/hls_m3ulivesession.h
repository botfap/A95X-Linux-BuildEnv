#ifndef __HLS_M3ULIVESESSION_H__
#define __HLS_M3ULIVESESSION_H__

/******************************************************************************

                  版权所有 (C), amlogic

 ******************************************************************************
  文 件 名   : hls_m3ulivesession.h
  版 本 号   : 初稿
  作    者   : peter
  生成日期   : 2013年2月21日 星期四
  最近修改   :
  功能描述   : hls_m3ulivesession.c 的头文件
  函数列表   :
  修改历史   :
  1.日    期   : 2013年2月21日 星期四
    作    者   : peter
    修改内容   : 创建文件

******************************************************************************/

/*----------------------------------------------*
 * 包含头文件                                   *
 *----------------------------------------------*/





/*----------------------------------------------*
 * 宏定义                                       *
 *----------------------------------------------*/





#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */


/*----------------------------------------------*
 * 外部变量说明                                 *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 外部函数原型说明                             *
 *----------------------------------------------*/



int m3u_session_open(const char* baseUrl,const char* headers,void** hSession);
int m3u_session_is_seekable(void* hSession);
int64_t m3u_session_seekUs(void* hSession,int64_t posUs,int (*interupt_func_cb)());

int m3u_session_get_durationUs(void*session,int64_t* dur);

int m3u_session_get_cached_data_time(void*session,int* time);

int m3u_session_get_estimate_bandwidth(void*session,int* bps);

int m3u_session_get_error_code(void*session,int* errcode);

int m3u_session_get_stream_num(void* session,int* num);
int m3u_session_get_cur_bandwidth(void* session,int* bw);

int m3u_session_set_codec_data(void* session,int time);

int m3u_session_read_data(void* session,void* buf,int len);

int m3u_session_close(void* hSession);

//ugly codes for cmf
int64_t m3u_session_get_next_segment_st(void* session);
int m3u_session_get_segment_num(void* session);
int64_t m3u_session_hybrid_seek(void* session,int64_t seg_st,int64_t pos,int (*interupt_func_cb)());
void* m3u_session_seek_by_index(void* session,int prev_index,int index,int (*interupt_func_cb)());
int64_t m3u_session_get_segment_size(void* session,const char* url,int index,int type);
void* m3u_session_get_index_by_timeUs(void* session,int64_t timeUs);
void* m3u_session_get_segment_info_by_index(void* hSession,int index);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif /* __HLS_M3ULIVESESSION_H__ */
