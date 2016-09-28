/***************************************************************************
 *  Copyright C 2009 by Amlogic, Inc. All Rights Reserved.
 */
/**\file
 * \brief 音视频解码模块内部头文件
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-08-06: create the document
 ***************************************************************************/

#ifndef _AM_AV_INTERNAL_H
#define _AM_AV_INTERNAL_H

#include <am_av.h>
#include <limits.h>
#include <pthread.h>

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

/****************************************************************************
 * Type definitions
 ***************************************************************************/

typedef struct AM_AV_Driver    AM_AV_Driver_t;
typedef struct AM_AV_Device    AM_AV_Device_t;
typedef struct AV_TSPlayer     AV_TSPlayer_t;
typedef struct AV_FilePlayer   AV_FilePlayer_t;
typedef struct AV_DataPlayer   AV_DataPlayer_t;

/**\brief TS流播放器*/
struct AV_TSPlayer
{
	void            *drv_data;       /**< 解码驱动相关数据*/
	uint16_t         aud_pid;        /**< 音频PID*/
	uint16_t         vid_pid;        /**< 视频PID*/
	aformat_t        aud_fmt;        /**< 音频压缩格式*/
	vformat_t        vid_fmt;        /**< 视频压缩格式*/
	AM_AV_TSSource_t src;            /**< TS源*/
};

/**\brief 文件播放器*/
struct AV_FilePlayer
{
	void            *drv_data;       /**< 解码驱动相关数据*/
	char             name[PATH_MAX]; /**< 文件名*/
	int              speed;          /**< 播放速度*/
};

/**\brief 数据播放参数*/
typedef struct
{
	const uint8_t  *data;            /**< 数据缓冲区指针*/
	int             len;             /**< 数据缓冲区长度*/
	int             times;           /**< 播放次数*/
	int             fd;              /**< 数据对应的文件描述符*/
	AM_Bool_t       need_free;       /**< 缓冲区播放完成后需要释放*/
	void           *para;            /**< 播放器相关参数指针*/
} AV_DataPlayPara_t;

/**\brief 数据播放器(ES, JPEG)*/
struct AV_DataPlayer
{
	void            *drv_data;       /**< 解码驱动相关数据*/
	AV_DataPlayPara_t para;          /**< 播放参数*/
};

/**\brief JPEG解码参数*/
typedef struct
{
	AM_OSD_Surface_t *surface;       /**< 返回JPEG图像*/
	AM_AV_JPEGAngle_t angle;         /**< 图片旋转角度*/
} AV_JPEGDecodePara_t;

/**\brief 播放模式*/
typedef enum
{
	AV_PLAY_STOP     = 0,            /**< 播放停止*/
	AV_PLAY_VIDEO_ES = 1,            /**< 播放视频ES流*/
	AV_PLAY_AUDIO_ES = 2,            /**< 播放音频ES流*/
	AV_PLAY_TS       = 4,            /**< TS流*/
	AV_PLAY_FILE     = 8,            /**< 播放文件*/
	AV_GET_JPEG_INFO = 16,           /**< 读取JPEG图片信息*/
	AV_DECODE_JPEG   = 32            /**< 解码JPEG图片*/
} AV_PlayMode_t;

/**\brief 播放命令*/
typedef enum
{
	AV_PLAY_START,                   /**< 开始播放*/
	AV_PLAY_PAUSE,                   /**< 暂停播放*/
	AV_PLAY_RESUME,                  /**< 恢复播放*/
	AV_PLAY_FF,                      /**< 快速前进*/
	AV_PLAY_FB,                      /**< 快速后退*/
	AV_PLAY_SEEK                     /**< 设定播放位置*/
} AV_PlayCmd_t;

/**\brief TS播放参数*/
typedef struct
{
	uint16_t        vpid;            /**< 视频流PID*/
	uint16_t        apid;            /**< 音频流PID*/
	vformat_t       vfmt;            /**< 视频流格式*/
	aformat_t       afmt;            /**< 音频流格式*/
} AV_TSPlayPara_t;

/**\brief 文件播放参数*/
typedef struct
{
	const char     *name;            /**< 文件名*/
	AM_Bool_t       loop;            /**< 是否循环播放*/
	AM_Bool_t       start;           /**< 是否开始播放*/
	int             pos;             /**< 开始播放的位置*/
} AV_FilePlayPara_t;

/**\brief 文件播放位置设定参数*/
typedef struct
{
	int             pos;             /**< 播放位置*/
	AM_Bool_t       start;           /**< 是否开始播放*/
} AV_FileSeekPara_t;

/**\brief 视频参数设置类型*/
typedef enum
{
	AV_VIDEO_PARA_WINDOW,
	AV_VIDEO_PARA_CONTRAST,
	AV_VIDEO_PARA_SATURATION,
	AV_VIDEO_PARA_BRIGHTNESS,
	AV_VIDEO_PARA_ENABLE,
	AV_VIDEO_PARA_BLACKOUT,
	AV_VIDEO_PARA_RATIO,
	AV_VIDEO_PARA_MODE
} AV_VideoParaType_t;

/**\brief 视频窗口参数*/
typedef struct
{
	int    x;
	int    y;
	int    w;
	int    h;
} AV_VideoWindow_t;

/**\brief 音视频解码驱动*/
struct AM_AV_Driver
{
	AM_ErrorCode_t (*open)(AM_AV_Device_t *dev, const AM_AV_OpenPara_t *para);
	AM_ErrorCode_t (*close)(AM_AV_Device_t *dev);
	AM_ErrorCode_t (*open_mode)(AM_AV_Device_t *dev, AV_PlayMode_t mode);
	AM_ErrorCode_t (*start_mode)(AM_AV_Device_t *dev, AV_PlayMode_t mode, void *para);
	AM_ErrorCode_t (*close_mode)(AM_AV_Device_t *dev, AV_PlayMode_t mode);
	AM_ErrorCode_t (*ts_source)(AM_AV_Device_t *dev, AM_AV_TSSource_t src);
	AM_ErrorCode_t (*file_cmd)(AM_AV_Device_t *dev, AV_PlayCmd_t cmd, void *data);
	AM_ErrorCode_t (*file_status)(AM_AV_Device_t *dev, AM_AV_PlayStatus_t *status);
	AM_ErrorCode_t (*file_info)(AM_AV_Device_t *dev, AM_AV_FileInfo_t *info);
	AM_ErrorCode_t (*set_video_para)(AM_AV_Device_t *dev, AV_VideoParaType_t para, void *val);
};

/**\brief 音视频解码设备*/
struct AM_AV_Device
{
	int              dev_no;         /**< 设备ID*/
	int              vout_dev_no;    /**< 视频解码设备对应的视频输出设备*/
	int              vout_w;         /**< 视频输出宽度*/
	int              vout_h;         /**< 视频输出高度*/
	const AM_AV_Driver_t  *drv;      /**< 解码驱动*/
	void            *drv_data;       /**< 解码驱动相关数据*/
	AV_TSPlayer_t    ts_player;      /**< TS流播放器*/
	AV_FilePlayer_t  file_player;    /**< 文件播放器*/
	AV_DataPlayer_t  vid_player;     /**< 视频数据播放器*/
	AV_DataPlayer_t  aud_player;     /**< 音频数据播放器*/
	pthread_mutex_t  lock;           /**< 设备资源保护互斥体*/
	AM_Bool_t        openned;        /**< 设备是否已经打开*/
	AV_PlayMode_t    mode;           /**< 当前播放模式*/
	int              video_x;        /**< 当前视频窗口左上角X坐标*/
	int              video_y;        /**< 当前视频窗口左上角Y坐标*/
	int              video_w;        /**< 当前视频窗口宽度*/
	int              video_h;        /**< 当前视频窗口高度*/
	int              video_contrast;       /**< 视频对比度*/
	int              video_saturation;     /**< 视频饱和度*/
	int              video_brightness;     /**< 视频亮度*/
	AM_Bool_t        video_enable;         /**< 视频层是否显示*/
	AM_Bool_t        video_blackout;       /**< 无数据时是否黑屏*/
	AM_AV_VideoAspectRatio_t video_ratio;  /**< 视频长宽比*/
	AM_AV_VideoDisplayMode_t video_mode;   /**< 视频显示模式*/
	AM_AV_SyncMode_t         sync_mode;    /**< A/V Sync Mode*/
	AM_AV_PCRRecoverMode_t   recover_mode; /**< A/V PCR Recover Mode*/
};

/****************************************************************************
 * Function prototypes  
 ***************************************************************************/


#ifdef __cplusplus
}
#endif

#endif

