/******************************************************************************

                  版权所有 (C), amlogic

 ******************************************************************************
  文 件 名   : hls_cmf_impl.h
  版 本 号   : 初稿
  作    者   : xiaoqiang.zhu
  生成日期   : 2013年4月23日 星期二
  最近修改   :
  功能描述   : hls_cmf_impl.c 的头文件
  函数列表   :
  修改历史   :
  1.日    期   : 2013年4月23日 星期二
    作    者   : xiaoqiang.zhu
    修改内容   : 创建文件

******************************************************************************/

/*----------------------------------------------*
 * 包含头文件                                   *
 *----------------------------------------------*/


/*----------------------------------------------*
 * 常量定义                                     *
 *----------------------------------------------*/



#ifndef __HLS_CMF_IMPL_H__
#define __HLS_CMF_IMPL_H__


#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */


#ifndef MAX_URL_SIZE
#define MAX_URL_SIZE 4096
#endif

typedef int (*interrupt_fun_t)(void) ;

typedef struct _CmfPrivContext{
    int totol_clip_num;
    int cur_clip_index;
    char cur_clip_path[MAX_URL_SIZE];
    int64_t cur_clip_st;
    int64_t cur_clip_end;
    int64_t cur_clip_offset;
    int64_t cur_clip_len;   
    interrupt_fun_t interrupt_func_cb;
}CmfPrivContext_t;


/*----------------------------------------------*
 * 外部函数原型说明                             *
 *----------------------------------------------*/

int hls_cmf_get_clip_num(void* session);
int64_t hls_cmf_seek_by_size(void* session,CmfPrivContext_t* ctx,int64_t pos);
int hls_cmf_shift_index_by_time(void* session,CmfPrivContext_t* ctx,int64_t posUs);
int64_t hls_cmf_seek_by_index(void* session,CmfPrivContext_t* ctx,int index);
int hls_cmf_get_fsize(void* session,CmfPrivContext_t* ctx,int type);//type = 0,common;type ==1,force

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif /* __HLS_CMF_IMPL_H__ */
