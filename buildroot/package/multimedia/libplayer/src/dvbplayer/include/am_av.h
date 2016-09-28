/***************************************************************************
 *  Copyright C 2009 by Amlogic, Inc. All Rights Reserved.
 */
/**\file
 * \brief 音视频解码驱动
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-08-06: create the document
 ***************************************************************************/

#ifndef _AM_AV_H
#define _AM_AV_H

#include "am_types.h"
#include "am_osd.h"
#include "am_evt.h"
#include "amports/vformat.h"
#include "amports/aformat.h"

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

/****************************************************************************
 * Error code definitions
 ****************************************************************************/

/**\brief 音视频解码模块错误代码*/
enum AM_AV_ErrorCode
{
	AM_AV_ERROR_BASE=AM_ERROR_BASE(AM_MOD_AV),
	AM_AV_ERR_INVALID_DEV_NO,          /**< 设备号无效*/
	AM_AV_ERR_BUSY,                    /**< 设备已经被打开*/
	AM_AV_ERR_ILLEGAL_OP,              /**< 无效的操作*/
	AM_AV_ERR_INVAL_ARG,               /**< 无效的参数*/
	AM_AV_ERR_NOT_ALLOCATED,           /**< 设备没有分配*/
	AM_AV_ERR_CANNOT_CREATE_THREAD,    /**< 无法创建线程*/
	AM_AV_ERR_CANNOT_OPEN_DEV,         /**< 无法打开设备*/
	AM_AV_ERR_CANNOT_OPEN_FILE,        /**< 无法打开文件*/
	AM_AV_ERR_NOT_SUPPORTED,           /**< 不支持的操作*/
	AM_AV_ERR_NO_MEM,                  /**< 空闲内存不足*/
	AM_AV_ERR_TIMEOUT,                 /**< 等待设备数据超时*/
	AM_AV_ERR_SYS,                     /**< 系统操作错误*/
	AM_AV_ERR_DECODE,                  /**< 解码出错*/
	AM_AV_ERR_END
};

/****************************************************************************
 * Event type definitions
 ****************************************************************************/

/**\brief 音视频模块事件类型*/
enum AM_AV_EventType
{
	AM_AV_EVT_BASE=AM_EVT_TYPE_BASE(AM_MOD_AV),
	AM_AV_EVT_PLAYER_STATE_CHANGED,    /**< 文件播放器状态发生改变,参数为AM_AV_MPState_t*/
	AM_AV_EVT_PLAYER_SPEED_CHANGED,    /**< 文件播放速度变化,参数为速度值(0为正常播放，<0表示回退播放，>0表示快速向前播放)*/
	AM_AV_EVT_PLAYER_TIME_CHANGED,     /**< 文件播放当前时间变化，参数为当前时间*/
	AM_AV_EVT_VIDEO_WINDOW_CHANGED,    /**< 视频窗口发生变化,参数为新的窗口大小(AM_AV_VideoWindow_t)*/
	AM_AV_EVT_VIDEO_CONTRAST_CHANGED,  /**< 视频对比度变化，参数为新对比度值(int 0~100)*/
	AM_AV_EVT_VIDEO_SATURATION_CHANGED,/**< 视频饱和度变化，参数为新饱和度值(int 0~100)*/
	AM_AV_EVT_VIDEO_BRIGHTNESS_CHANGED,/**< 视频亮度变化，参数为新亮度值(int 0~100)*/
	AM_AV_EVT_VIDEO_ENABLED,           /**< 视频层显示*/
	AM_AV_EVT_VIDEO_DISABLED,          /**< 视频曾隐藏*/
	AM_AV_EVT_VIDEO_ASPECT_RATIO_CHANGED, /**< 视频长宽比变化，参数为新的长宽比(AM_AV_VideoAspectRatio_t)*/
	AM_AV_EVT_VIDEO_DISPLAY_MODE_CHANGED, /**< 视频显示模式变化，参数为新的显示模式(AM_AV_VideoDisplayMode_t)*/
	AM_AV_EVT_SYNC_MODE_CHANGED,          /**< Sync模式变化，参数为新的Sync模式(AM_AV_SyncMode_t)*/
  AM_AV_EVT_PCRRECOVER_MODE_CHANGED,    /**< PCR Recover模式变化，参数为新的Sync模式(AM_AV_PCRRecoverMode_t)*/
	AM_AV_EVT_END
};


/****************************************************************************
 * Type definitions
 ***************************************************************************/

/**\brief 文件播放器状态*/
typedef enum
{
	AM_AV_MP_STATE_UNKNOWN = 0,        /**< 未知状态*/
	AM_AV_MP_STATE_INITING,            /**< 正在进行初始化*/
	AM_AV_MP_STATE_NORMALERROR,        /**< 发生错误*/
	AM_AV_MP_STATE_FATALERROR,         /**< 发生致命错误*/
	AM_AV_MP_STATE_PARSERED,           /**< 文件信息分析完毕*/
	AM_AV_MP_STATE_STARTED,            /**< 开始播放*/
	AM_AV_MP_STATE_PLAYING,            /**< 正在播放*/
	AM_AV_MP_STATE_PAUSED,             /**< 暂停播放*/
	AM_AV_MP_STATE_CONNECTING,         /**< 正在连接服务器*/
	AM_AV_MP_STATE_CONNECTDONE,        /**< 服务器连接完毕*/
	AM_AV_MP_STATE_BUFFERING,          /**< 正在缓存数据*/
	AM_AV_MP_STATE_BUFFERINGDONE,      /**< 数据缓存结束*/
	AM_AV_MP_STATE_SEARCHING,          /**< 正在设定播放位置*/
	AM_AV_MP_STATE_TRICKPLAY,          /**< 正在进行trick mode播放*/
	AM_AV_MP_STATE_MESSAGE_EOD,        /**< 消息结束*/
	AM_AV_MP_STATE_MESSAGE_BOD,        /**< 消息开始*/
	AM_AV_MP_STATE_TRICKTOPLAY,        /**< 从trick mode切换到play模式*/
	AM_AV_MP_STATE_FINISHED,           /**< 播放完毕*/
	AM_AV_MP_STATE_STOPED              /**< 播放器停止*/
} AM_AV_MPState_t;

/**\brief 视频窗口*/
typedef struct
{
	int x;                               /**< 窗口左上角X坐标*/
	int y;                               /**< 窗口左上角Y坐标*/
	int w;                               /**< 窗口宽度*/
	int h;                               /**< 窗口高度*/
} AM_AV_VideoWindow_t;

/**\brief 音视频播放的TS流输入源*/
typedef enum
{
	AM_AV_TS_SRC_TS0,                    /**< TS输入0*/
	AM_AV_TS_SRC_TS1,                    /**< TS输入1*/
	AM_AV_TS_SRC_TS2,                    /**< TS输入1*/	
	AM_AV_TS_SRC_HIU                     /**< HIU 接口*/
} AM_AV_TSSource_t;

/**\brief 视频长宽比*/
typedef enum
{
	AM_AV_VIDEO_ASPECT_AUTO,      /**< 自动设定长宽比*/
	AM_AV_VIDEO_ASPECT_4_3,       /**< 4:3*/
	AM_AV_VIDEO_ASPECT_16_9       /**< 16:9*/
} AM_AV_VideoAspectRatio_t;

/**\brief 视频屏幕显示模式*/
typedef enum
{
	AM_AV_VIDEO_DISPLAY_NORMAL,     /**< 普通显示*/
	AM_AV_VIDEO_DISPLAY_FULL_SCREEN /**< 全屏显示*/
} AM_AV_VideoDisplayMode_t;

/**\brief A/V Sync mode*/
typedef enum
{
  AM_AV_SYNC_DISABLE,     /**< Disable A/V sync*/
  AM_AV_SYNC_ENABLE       /**< Enable A/V sync*/
} AM_AV_SyncMode_t;

/**\brief PCR Recover mode*/
typedef enum
{
  AM_AV_RECOVER_DISABLE,     /**< Disable PCR recover*/
  AM_AV_RECOVER_ENABLE       /**< Enable PCR recover*/
} AM_AV_PCRRecoverMode_t;

/**\brief 音视频解码设备开启参数*/
typedef struct
{
	int      vout_dev_no;         /**< 音视频播放设备对应的视频输出设备ID*/
} AM_AV_OpenPara_t;

/**\brief 媒体文件信息*/
typedef struct
{
	uint64_t size;                /**< 文件大小*/ 
	int      duration;            /**< 总播放时长*/
} AM_AV_FileInfo_t;

/**\brief 文件播放状态*/
typedef struct
{
	int      duration;            /**< 总播放时长*/
	int      position;            /**< 当前播放时间*/
} AM_AV_PlayStatus_t;

/**\brief JPEG文件属性*/
typedef struct
{
	int      width;               /**< JPEG宽度*/
	int      height;              /**< JPEG高度*/
	int      comp_num;            /**< 颜色数*/
} AM_AV_JPEGInfo_t;

/**\brief JPEG旋转角度*/
typedef enum
{
	AM_AV_JPEG_CLKWISE_0    = 0,  /**< 不旋转*/
        AM_AV_JPEG_CLKWISE_90   = 1,  /**< 顺时针旋转90度*/
        AM_AV_JPEG_CLKWISE_180  = 2,  /**< 顺时针旋转180度*/
        AM_AV_JPEG_CLKWISE_270  = 3   /**< 顺时针旋转270度*/
} AM_AV_JPEGAngle_t;

/****************************************************************************
 * Function prototypes  
 ***************************************************************************/

/**\brief 打开音视频解码设备
 * \param dev_no 音视频设备号
 * \param[in] para 音视频设备开启参数
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
extern AM_ErrorCode_t AM_AV_Open(int dev_no, const AM_AV_OpenPara_t *para);

/**\brief 关闭音视频解码设备
 * \param dev_no 音视频设备号
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
extern AM_ErrorCode_t AM_AV_Close(int dev_no);

/**\brief 设定TS流的输入源
 * \param dev_no 音视频设备号
 * \param src TS输入源
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
extern AM_ErrorCode_t AM_AV_SetTSSource(int dev_no, AM_AV_TSSource_t src);

/**\brief 开始解码TS流
 * \param dev_no 音视频设备号
 * \param vpid 视频流PID
 * \param apid 音频流PID
 * \param vfmt 视频压缩格式
 * \param afmt 音频压缩格式
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
extern AM_ErrorCode_t AM_AV_StartTS(int dev_no, uint16_t vpid, uint16_t apid, vformat_t vfmt, aformat_t afmt);

/**\brief 停止TS流解码
 * \param dev_no 音视频设备号
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
extern AM_ErrorCode_t AM_AV_StopTS(int dev_no);

/**\brief 开始播放文件
 * \param dev_no 音视频设备号
 * \param[in] fname 媒体文件名
 * \param loop 是否循环播放
 * \param pos 播放的起始位置
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
extern AM_ErrorCode_t AM_AV_StartFile(int dev_no, const char *fname, AM_Bool_t loop, int pos);

/**\brief 切换到文件播放模式但不开始播放
 * \param dev_no 音视频设备号
 * \param[in] fname 媒体文件名
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
extern AM_ErrorCode_t AM_AV_AddFile(int dev_no, const char *fname);

/**\brief 开始播放已经添加的文件，在AM_AV_AddFile后调用此函数开始播放
 * \param dev_no 音视频设备号
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
extern AM_ErrorCode_t AM_AV_StartCurrFile(int dev_no);

/**\brief 停止播放文件
 * \param dev_no 音视频设备号
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
extern AM_ErrorCode_t AM_AV_StopFile(int dev_no);

/**\brief 暂停文件播放
 * \param dev_no 音视频设备号
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
extern AM_ErrorCode_t AM_AV_PauseFile(int dev_no);

/**\brief 恢复文件播放
 * \param dev_no 音视频设备号
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
extern AM_ErrorCode_t AM_AV_ResumeFile(int dev_no);

/**\brief 设定当前文件播放位置
 * \param dev_no 音视频设备号
 * \param pos 播放位置
 * \param start 是否开始播放
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
extern AM_ErrorCode_t AM_AV_SeekFile(int dev_no, int pos, AM_Bool_t start);

/**\brief 快速向前播放
 * \param dev_no 音视频设备号
 * \param speed 播放速度
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
extern AM_ErrorCode_t AM_AV_FastForwardFile(int dev_no, int speed);

/**\brief 快速向后播放
 * \param dev_no 音视频设备号
 * \param speed 播放速度
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
extern AM_ErrorCode_t AM_AV_FastBackwardFile(int dev_no, int speed);

/**\brief 取得当前媒体文件信息
 * \param dev_no 音视频设备号
 * \param[out] info 返回媒体文件信息
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
extern AM_ErrorCode_t AM_AV_GetCurrFileInfo(int dev_no, AM_AV_FileInfo_t *info);

/**\brief 取得媒体文件播放状态
 * \param dev_no 音视频设备号
 * \param[out] status 返回媒体文件播放状态
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
extern AM_ErrorCode_t AM_AV_GetPlayStatus(int dev_no, AM_AV_PlayStatus_t *status);

/**\brief 取得JPEG图片文件属性
 * \param dev_no 音视频设备号
 * \param[in] fname 图片文件名
 * \param[out] info 文件属性
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
extern AM_ErrorCode_t AM_AV_GetJPEGInfo(int dev_no, const char *fname, AM_AV_JPEGInfo_t *info);

/**\brief 取得JPEG图片数据属性
 * \param dev_no 音视频设备号
 * \param[in] data 图片数据缓冲区
 * \param len 数据缓冲区大小
 * \param[out] info 文件属性
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
extern AM_ErrorCode_t AM_AV_GetJPEGDataInfo(int dev_no, const uint8_t *data, int len, AM_AV_JPEGInfo_t *info);

/**\brief 解码JPEG图片文件
 * \param dev_no 音视频设备号
 * \param[in] fname 图片文件名
 * \param angle 图片旋转角度
 * \param[out] surf 返回JPEG图片的surface
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
extern AM_ErrorCode_t AM_AV_DecodeJPEG(int dev_no, const char *fname, AM_AV_JPEGAngle_t angle, AM_OSD_Surface_t **surf);

/**\brief 解码JPEG图片数据
 * \param dev_no 音视频设备号
 * \param[in] data 图片数据缓冲区
 * \param len 数据缓冲区大小
 * \param angle 图片旋转角度
 * \param[out] surf 返回JPEG图片的surface
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
extern AM_ErrorCode_t AM_AV_DacodeJPEGData(int dev_no, const uint8_t *data, int len, AM_AV_JPEGAngle_t angle, AM_OSD_Surface_t **surf);

/**\brief 解码视频基础流文件
 * \param dev_no 音视频设备号
 * \param format 视频压缩格式
 * \param[in] fname 视频文件名
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
extern AM_ErrorCode_t AM_AV_StartVideoES(int dev_no, vformat_t format, const char *fname);

/**\brief 解码视频基础流数据
 * \param dev_no 音视频设备号
 * \param format 视频压缩格式
 * \param[in] data 视频数据缓冲区
 * \param len 数据缓冲区大小
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
extern AM_ErrorCode_t AM_AV_StartVideoESData(int dev_no, vformat_t format, const uint8_t *data, int len);

/**\brief 解码音频基础流文件
 * \param dev_no 音视频设备号
 * \param format 音频压缩格式
 * \param[in] fname 视频文件名
 * \param times 播放次数，<=0表示一直播放
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
extern AM_ErrorCode_t AM_AV_StartAudioES(int dev_no, aformat_t format, const char *fname, int times);

/**\brief 解码音频基础流数据
 * \param dev_no 音视频设备号
 * \param format 音频压缩格式
 * \param[in] data 音频数据缓冲区
 * \param len 数据缓冲区大小
 * \param times 播放次数，<=0表示一直播放
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
extern AM_ErrorCode_t AM_AV_StartAudioESData(int dev_no, aformat_t format, const uint8_t *data, int len, int times);

/**\brief 停止解码音频基础流
 * \param dev_no 音视频设备号
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
extern AM_ErrorCode_t AM_AV_StopAudioES(int dev_no);

/**\brief 设定视频窗口
 * \param dev_no 音视频设备号
 * \param x 窗口左上顶点x坐标
 * \param y 窗口左上顶点y坐标
 * \param w 窗口宽度
 * \param h 窗口高度
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
extern AM_ErrorCode_t AM_AV_SetVideoWindow(int dev_no, int x, int y, int w, int h);

/**\brief 返回视频窗口
 * \param dev_no 音视频设备号
 * \param[out] x 返回窗口左上顶点x坐标
 * \param[out] y 返回窗口左上顶点y坐标
 * \param[out] w 返回窗口宽度
 * \param[out] h 返回窗口高度
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
extern AM_ErrorCode_t AM_AV_GetVideoWindow(int dev_no, int *x, int *y, int *w, int *h);

/**\brief 设定视频对比度(0~100)
 * \param dev_no 音视频设备号
 * \param val 视频对比度值
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
extern AM_ErrorCode_t AM_AV_SetVideoContrast(int dev_no, int val);

/**\brief 取得当前视频对比度
 * \param dev_no 音视频设备号
 * \param[out] val 返回当前视频对比度值
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
extern AM_ErrorCode_t AM_AV_GetVideoContrast(int dev_no, int *val);

/**\brief 设定视频饱和度(0~100)
 * \param dev_no 音视频设备号
 * \param val 视频饱和度值
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
extern AM_ErrorCode_t AM_AV_SetVideoSaturation(int dev_no, int val);

/**\brief 取得当前视频饱和度
 * \param dev_no 音视频设备号
 * \param[out] val 返回当前视频饱和度值
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
extern AM_ErrorCode_t AM_AV_GetVideoSaturation(int dev_no, int *val);

/**\brief 设定视频亮度(0~100)
 * \param dev_no 音视频设备号
 * \param val 视频亮度值
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
extern AM_ErrorCode_t AM_AV_SetVideoBrightness(int dev_no, int val);

/**\brief 取得当前视频亮度
 * \param dev_no 音视频设备号
 * \param[out] val 返回当前视频亮度值
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
extern AM_ErrorCode_t AM_AV_GetVideoBrightness(int dev_no, int *val);

/**\brief 显示视频层
 * \param dev_no 音视频设备号
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
extern AM_ErrorCode_t AM_AV_EnableVideo(int dev_no);

/**\brief 隐藏视频层
 * \param dev_no 音视频设备号
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
extern AM_ErrorCode_t AM_AV_DisableVideo(int dev_no);

/**\brief 设定视频长宽比
 * \param dev_no 音视频设备号
 * \param ratio 长宽比
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
extern AM_ErrorCode_t AM_AV_SetVideoAspectRatio(int dev_no, AM_AV_VideoAspectRatio_t ratio);

/**\brief 取得视频长宽比
 * \param dev_no 音视频设备号
 * \param[out] ratio 长宽比
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
extern AM_ErrorCode_t AM_AV_GetVideoAspectRatio(int dev_no, AM_AV_VideoAspectRatio_t *ratio);

/**\brief 设定视频停止时自动隐藏视频层
 * \param dev_no 音视频设备号
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
extern AM_ErrorCode_t AM_AV_EnableVideoBlackout(int dev_no);

/**\brief 视频停止时不隐藏视频层
 * \param dev_no 音视频设备号
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
extern AM_ErrorCode_t AM_AV_DisableVideoBlackout(int dev_no);

/**\brief 设定视频显示模式
 * \param dev_no 音视频设备号
 * \param mode 视频显示模式
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
extern AM_ErrorCode_t AM_AV_SetVideoDisplayMode(int dev_no, AM_AV_VideoDisplayMode_t mode);

/**\brief 取得当前视频显示模式
 * \param dev_no 音视频设备号
 * \param mode 返回当前视频显示模式
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
extern AM_ErrorCode_t AM_AV_GetVideoDisplayMode(int dev_no, AM_AV_VideoDisplayMode_t *mode);

/**\brief Set A/V sync mode
 * \param dev_no 音视频设备号
 * \param mode sync mode
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
extern AM_ErrorCode_t AM_AV_SetSyncMode(int dev_no, AM_AV_SyncMode_t mode);

/**\brief Get A/V sync mode
 * \param dev_no 音视频设备号
 * \param mode sync mode
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
extern AM_ErrorCode_t AM_AV_GetSyncMode(int dev_no, AM_AV_SyncMode_t *mode);

/**\brief Set PCR recover mode
 * \param dev_no 音视频设备号
 * \param mode PCR Recover mode
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
AM_ErrorCode_t AM_AV_SetPCRRecoverMode(int dev_no, AM_AV_PCRRecoverMode_t mode);

/**\brief Get PCR Recover mode
 * \param dev_no 音视频设备号
 * \param mode PCR Recover mode
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
extern AM_ErrorCode_t AM_AV_GetPCRRecoverMode(int dev_no, AM_AV_PCRRecoverMode_t *mode);

#ifdef __cplusplus
}
#endif

#endif

