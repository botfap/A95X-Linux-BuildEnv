#ifndef HLS_BANDWIDTH_MEASURE_H
#define HLS_BANDWIDTH_MEASURE_H

/******************************************************************************

                  版权所有 (C), amlogic

 ******************************************************************************
  文 件 名   : hls_m3ulivesession.h
  版 本 号   : 初稿
  作    者   : zz
  生成日期   : 2013年2月21日 星期四
  最近修改   :
  功能描述   : hls_bandwidth_measure.c 的头文件
  函数列表   :
  修改历史   :
  1.日    期   : 2013年2月21日 星期四
    作    者   : xiaoqiang.zhu
    修改内容   : 修改函数，适配应用。

******************************************************************************/
#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */

void *bandwidth_measure_alloc(int measurenum,int flags);
int bandwidth_measure_add(void *band,int bytes,int delay_ns);
int bandwidth_measure_get_bandwidth(void  *band,int *fast_band,int *mid_band,int *avg_band);
void bandwidth_measure_start_read(void *band);
int bandwidth_measure_finish_read(void *band,int bytes);
int bandwidth_measure_free(void *band);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif
