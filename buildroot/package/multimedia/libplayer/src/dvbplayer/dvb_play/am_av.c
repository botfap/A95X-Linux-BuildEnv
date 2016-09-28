/***************************************************************************
 *  Copyright C 2009 by Amlogic, Inc. All Rights Reserved.
 */
/**\file
 * \brief 音视频解码模块
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-08-06: create the document
 ***************************************************************************/

#define AM_DEBUG_LEVEL 5

#include <am_debug.h>
#include <am_vout.h>
#include <am_mem.h>
#include "am_av_internal.h"
//#include "../am_adp_internal.h"
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include "log_print.h"

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

#define AV_DEV_COUNT      (1)

/****************************************************************************
 * Static data
 ***************************************************************************/

#ifdef EMU_AV
extern const AM_AV_Driver_t emu_av_drv;
#else
extern const AM_AV_Driver_t aml_av_drv;
#endif

static AM_AV_Device_t av_devices[AV_DEV_COUNT] =
{
#ifdef EMU_AV
{
.drv = &emu_av_drv
}
#else
{
.drv = &aml_av_drv
}
#endif
};


extern pthread_mutex_t am_gAdpLock;

/****************************************************************************
 * Static functions
 ***************************************************************************/
 
/**\brief 根据设备号取得设备结构指针*/
static AM_INLINE AM_ErrorCode_t av_get_dev(int dev_no, AM_AV_Device_t **dev)
{
	if((dev_no<0) || (dev_no>=AV_DEV_COUNT))
	{
		AM_DEBUG(1, "invalid AV device number %d, must in(%d~%d)", dev_no, 0, AV_DEV_COUNT-1);
		return AM_AV_ERR_INVALID_DEV_NO;
	}
	
	*dev = &av_devices[dev_no];
	return AM_SUCCESS;
}

/**\brief 根据设备号取得设备结构并检查设备是否已经打开*/
static AM_INLINE AM_ErrorCode_t av_get_openned_dev(int dev_no, AM_AV_Device_t **dev)
{
	AM_TRY(av_get_dev(dev_no, dev));
	
	if(!(*dev)->openned)
	{
		AM_DEBUG(1, "AV device %d has not been openned", dev_no);
		return AM_AV_ERR_INVALID_DEV_NO;
	}
	
	return AM_SUCCESS;
}

/**\brief 释放数据参数中的相关资源*/
static void av_free_data_para(AV_DataPlayPara_t *para)
{
	if(para->need_free)
	{
		munmap((uint8_t*)para->data, para->len);
		close(para->fd);
		para->need_free = AM_FALSE;
	}
}

/**\brief 结束播放*/
static AM_ErrorCode_t av_stop(AM_AV_Device_t *dev, AV_PlayMode_t mode)
{
	if(!(dev->mode&mode))
		return AM_SUCCESS;
	
	if(dev->drv->close_mode)
		dev->drv->close_mode(dev, mode);
	
	switch(mode)
	{
		case AV_PLAY_VIDEO_ES:
		case AV_DECODE_JPEG:
		case AV_GET_JPEG_INFO:
			av_free_data_para(&dev->vid_player.para);
		break;
		case AV_PLAY_AUDIO_ES:
			av_free_data_para(&dev->aud_player.para);
		break;
		default:
		break;
	}
	
	dev->mode &= ~mode;
	return AM_SUCCESS;
}

/**\brief 结束所有播放*/
static AM_ErrorCode_t av_stop_all(AM_AV_Device_t *dev, AV_PlayMode_t mode)
{
	if(mode&AV_PLAY_VIDEO_ES)
		av_stop(dev, AV_PLAY_VIDEO_ES);
	if(mode&AV_PLAY_AUDIO_ES)
		av_stop(dev, AV_PLAY_AUDIO_ES);
	if(mode&AV_GET_JPEG_INFO)
		av_stop(dev, AV_GET_JPEG_INFO);
	if(mode&AV_DECODE_JPEG)
		av_stop(dev, AV_DECODE_JPEG);
	if(mode&AV_PLAY_TS)
		av_stop(dev, AV_PLAY_TS);
	if(mode&AV_PLAY_FILE)
		av_stop(dev, AV_PLAY_FILE);
	
	return AM_SUCCESS;
}

/**\brief 开始播放*/
static AM_ErrorCode_t av_start(AM_AV_Device_t *dev, AV_PlayMode_t mode, void *para)
{
	AM_ErrorCode_t ret = AM_SUCCESS;
	AV_PlayMode_t stop_mode;
	AV_DataPlayPara_t *dp_para;
	
	/*检查需要停止的播放模式*/
	switch(mode)
	{
		case AV_PLAY_VIDEO_ES:
			stop_mode = AV_DECODE_JPEG|AV_GET_JPEG_INFO|AV_PLAY_TS|AV_PLAY_FILE;
		break;
		case AV_PLAY_AUDIO_ES:
			stop_mode = AV_PLAY_TS|AV_PLAY_FILE|AV_PLAY_AUDIO_ES;
		break;
		case AV_GET_JPEG_INFO:
			stop_mode = AV_DECODE_JPEG|AV_GET_JPEG_INFO|AV_PLAY_VIDEO_ES|AV_PLAY_TS|AV_PLAY_FILE;
		break;
		case AV_DECODE_JPEG:
			stop_mode = AV_DECODE_JPEG|AV_GET_JPEG_INFO|AV_PLAY_VIDEO_ES|AV_PLAY_TS|AV_PLAY_FILE;
		break;
		case AV_PLAY_TS:
			stop_mode = AV_DECODE_JPEG|AV_GET_JPEG_INFO|AV_PLAY_VIDEO_ES|AV_PLAY_AUDIO_ES|AV_PLAY_FILE|AV_PLAY_TS;
		break;
		case AV_PLAY_FILE:
			stop_mode = AV_DECODE_JPEG|AV_GET_JPEG_INFO|AV_PLAY_AUDIO_ES|AV_PLAY_VIDEO_ES|AV_PLAY_TS|AV_PLAY_FILE;
		break;
		default:
			stop_mode = 0;
		break;
	}
	
	if(stop_mode)
		av_stop_all(dev, stop_mode);
	
	if(dev->drv->open_mode && !(dev->mode&mode))
	{
		ret = dev->drv->open_mode(dev, mode);
		if(ret!=AM_SUCCESS) {
			log_print("dev->drv->open_mode failed\n");	
			return ret;
		}
	}
	
	dev->mode |= mode;
	
	/*记录相关参数数据*/
	switch(mode)
	{
		case AV_PLAY_VIDEO_ES:
		case AV_GET_JPEG_INFO:
		case AV_DECODE_JPEG:
			dp_para = (AV_DataPlayPara_t*)para;
			dev->vid_player.para = *dp_para;
			para = dp_para->para;
		break;
		case AV_PLAY_AUDIO_ES:
			dp_para = (AV_DataPlayPara_t*)para;
			dev->aud_player.para = *dp_para;
			para = dp_para->para;
		break;
		default:
		break;
	}
	
	/*开始播放*/
	if(dev->drv->start_mode)
	{
		log_print("dev->drv->start_mode\n");	
		ret = dev->drv->start_mode(dev, mode, para);
		if(ret!=AM_SUCCESS)
		{
			log_print("dev->drv->start_mode failed\n");	
			switch(mode)
			{
				case AV_PLAY_VIDEO_ES:
				case AV_GET_JPEG_INFO:
				case AV_DECODE_JPEG:
					dev->vid_player.para.need_free = AM_FALSE;
				break;
				case AV_PLAY_AUDIO_ES:
					dev->aud_player.para.need_free = AM_FALSE;
				break;
				default:
				break;
			}
			return ret;
		}
	}
	
	return AM_SUCCESS;
}

/**\brief 视频输出模式变化事件回调*/
static void av_vout_format_changed(int dev_no, int event_type, void *param, void *data)
{
	AM_AV_Device_t *dev = (AM_AV_Device_t*)data;
	
	if(event_type==AM_VOUT_EVT_FORMAT_CHANGED)
	{
		AM_VOUT_Format_t fmt = (AM_VOUT_Format_t)param;
		int w, h;
		
		w = dev->vout_w;
		h = dev->vout_h;
		
		switch(fmt)
		{
			case AM_VOUT_FORMAT_576CVBS:
				w = 720;
				h = 576;
			break;
			case AM_VOUT_FORMAT_480CVBS:
				w = 720;
				h = 480;
			break;
			case AM_VOUT_FORMAT_576I:
				w = 720;
				h = 576;
			break;
			case AM_VOUT_FORMAT_576P:
				w = 720;
				h = 576;
			break;
			case AM_VOUT_FORMAT_480I:
				w = 720;
				h = 480;
			break;
			case AM_VOUT_FORMAT_480P:
				w = 720;
				h = 480;
			break;
			case AM_VOUT_FORMAT_720P:
				w = 1280;
				h = 720;
			break;
			case AM_VOUT_FORMAT_1080I:
				w = 1920;
				h = 1080;
			break;
			case AM_VOUT_FORMAT_1080P:
				w = 1920;
				h = 1080;
			break;
			default:
			break;
		}
		
		if((w!=dev->vout_w) || (h!=dev->vout_h))
		{
			int nx, ny, nw, nh;
			
			nx = dev->video_x*w/dev->vout_w;
			ny = dev->video_y*h/dev->vout_h;
			nw = dev->video_w*w/dev->vout_w;
			nh = dev->video_h*h/dev->vout_h;
			
			AM_AV_SetVideoWindow(dev->dev_no, nx, ny, nw, nh);
			
			dev->vout_w = w;
			dev->vout_h = h;
		}
	}
}

/****************************************************************************
 * API functions
 ***************************************************************************/

/**\brief 打开音视频解码设备
 * \param dev_no 音视频设备号
 * \param[in] para 音视频设备开启参数
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
AM_ErrorCode_t AM_AV_Open(int dev_no, const AM_AV_OpenPara_t *para)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	assert(para);
	
	AM_TRY(av_get_dev(dev_no, &dev));
	
	pthread_mutex_lock(&am_gAdpLock);
	
	if(dev->openned)
	{
		AM_DEBUG(1, "AV device %d has already been openned", dev_no);
		ret = AM_AV_ERR_BUSY;
		goto final;
	}
	
	dev->dev_no  = dev_no;
	pthread_mutex_init(&dev->lock, NULL);
	dev->openned = AM_TRUE;
	dev->mode    = 0;
	dev->video_x = 0;
	dev->video_y = 0;
	dev->video_w = 1280;
	dev->video_h = 720;
	dev->video_contrast   = 0;
	dev->video_saturation = 0;
	dev->video_brightness = 0;
	dev->video_enable     = AM_TRUE;
	dev->video_blackout   = AM_TRUE;
	dev->video_ratio      = AM_AV_VIDEO_ASPECT_AUTO;
	dev->video_mode       = AM_AV_VIDEO_DISPLAY_NORMAL;
	dev->vout_dev_no      = para->vout_dev_no;
	dev->vout_w           = 0;
	dev->vout_h           = 0;
	
	if(dev->drv->open)
	{
		ret = dev->drv->open(dev, para);
		if(ret!=AM_SUCCESS)
			goto final;
	}
	
	if(ret==AM_SUCCESS)
	{
		AM_EVT_Subscribe(dev->vout_dev_no, AM_VOUT_EVT_FORMAT_CHANGED, av_vout_format_changed, dev);
	}
	
final:
	pthread_mutex_unlock(&am_gAdpLock);
	
	return ret;
}

/**\brief 关闭音视频解码设备
 * \param dev_no 音视频设备号
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
AM_ErrorCode_t AM_AV_Close(int dev_no)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	AM_TRY(av_get_openned_dev(dev_no, &dev));
	
	pthread_mutex_lock(&am_gAdpLock);
	
	/*注销事件*/
	AM_EVT_Unsubscribe(dev->vout_dev_no, AM_VOUT_EVT_FORMAT_CHANGED, av_vout_format_changed, dev);
	
	/*停止播放*/
	av_stop_all(dev, dev->mode);
	
	if(dev->drv->close)
		dev->drv->close(dev);
	
	/*释放资源*/
	pthread_mutex_destroy(&dev->lock);
	
	dev->openned = AM_FALSE;
	
	pthread_mutex_unlock(&am_gAdpLock);
	
	return ret;
}

/**\brief 设定TS流的输入源
 * \param dev_no 音视频设备号
 * \param src TS输入源
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
AM_ErrorCode_t AM_AV_SetTSSource(int dev_no, AM_AV_TSSource_t src)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	AM_TRY(av_get_openned_dev(dev_no, &dev));
	
	pthread_mutex_lock(&dev->lock);
	
	if(dev->drv->ts_source)
		ret = dev->drv->ts_source(dev, src);
	
	if(ret==AM_SUCCESS)
		dev->ts_player.src = src;
	
	pthread_mutex_unlock(&dev->lock);
	
	return ret;
}

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
AM_ErrorCode_t AM_AV_StartTS(int dev_no, uint16_t vpid, uint16_t apid, vformat_t vfmt, aformat_t afmt)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	AV_TSPlayPara_t para;
	
	AM_TRY(av_get_openned_dev(dev_no, &dev));
	
	pthread_mutex_lock(&dev->lock);
	
	para.vpid = vpid;
	para.vfmt = vfmt;
	para.apid = apid;
	para.afmt = afmt;
	log_print("***********************\n");

  pthread_mutex_unlock(&dev->lock);

  if(para.vpid && para.apid)
    AM_AV_SetSyncMode(dev_no, AM_AV_SYNC_ENABLE);
  else
    AM_AV_SetSyncMode(dev_no, AM_AV_SYNC_DISABLE);

  AM_AV_SetPCRRecoverMode(dev_no, AM_AV_RECOVER_ENABLE);

  pthread_mutex_lock(&dev->lock);
  ret = av_start(dev, AV_PLAY_TS, &para);
	pthread_mutex_unlock(&dev->lock);
	
	return ret;
}

/**\brief 停止TS流解码
 * \param dev_no 音视频设备号
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
AM_ErrorCode_t AM_AV_StopTS(int dev_no)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	AM_TRY(av_get_openned_dev(dev_no, &dev));
	
	pthread_mutex_lock(&dev->lock);
	
	ret = av_stop(dev, AV_PLAY_TS);
	
	pthread_mutex_unlock(&dev->lock);

  AM_AV_SetSyncMode(dev_no, AM_AV_SYNC_DISABLE);
  AM_AV_SetPCRRecoverMode(dev_no, AM_AV_RECOVER_DISABLE);
	
	return ret;
}

/**\brief 开始播放文件
 * \param dev_no 音视频设备号
 * \param[in] fname 媒体文件名
 * \param loop 是否循环播放
 * \param pos 播放的起始位置
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
AM_ErrorCode_t AM_AV_StartFile(int dev_no, const char *fname, AM_Bool_t loop, int pos)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	AV_FilePlayPara_t para;
	
	assert(fname);
	
	AM_TRY(av_get_openned_dev(dev_no, &dev));
	
	pthread_mutex_lock(&dev->lock);
	
	para.name = fname;
	para.loop = loop;
	para.pos  = pos;
	para.start= AM_TRUE;
	
	ret = av_start(dev, AV_PLAY_FILE, &para);
	if(ret==AM_SUCCESS)
	{
		snprintf(dev->file_player.name, sizeof(dev->file_player.name), "%s", fname);
		dev->file_player.speed = 0;
	}
	
	pthread_mutex_unlock(&dev->lock);
	
	return ret;
}

/**\brief 切换到文件播放模式但不开始播放
 * \param dev_no 音视频设备号
 * \param[in] fname 媒体文件名
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
AM_ErrorCode_t AM_AV_AddFile(int dev_no, const char *fname)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	AV_FilePlayPara_t para;
	
	assert(fname);
	
	AM_TRY(av_get_openned_dev(dev_no, &dev));
	
	pthread_mutex_lock(&dev->lock);
	
	para.name = fname;
	para.start= AM_FALSE;
	
	ret = av_start(dev, AV_PLAY_FILE, &para);
	if(ret==AM_SUCCESS)
	{
		snprintf(dev->file_player.name, sizeof(dev->file_player.name), "%s", fname);
		dev->file_player.speed = 0;
	}
	
	pthread_mutex_unlock(&dev->lock);
	
	return ret;
}

/**\brief 开始播放已经添加的文件，在AM_AV_AddFile后调用此函数开始播放
 * \param dev_no 音视频设备号
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
AM_ErrorCode_t AM_AV_StartCurrFile(int dev_no)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	AM_TRY(av_get_openned_dev(dev_no, &dev));
	
	pthread_mutex_lock(&dev->lock);
	
	if(!(dev->mode&AV_PLAY_FILE))
	{
		AM_DEBUG(1, "do not in the file play mode");
		ret = AM_AV_ERR_ILLEGAL_OP;
	}
	
	if(ret==AM_SUCCESS)
	{
		if(dev->drv->file_cmd)
		{
			ret = dev->drv->file_cmd(dev, AV_PLAY_START, 0);
		}
	}
	
	pthread_mutex_unlock(&dev->lock);
	
	return ret;
}

/**\brief 停止播放文件
 * \param dev_no 音视频设备号
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
AM_ErrorCode_t AM_AV_StopFile(int dev_no)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	AM_TRY(av_get_openned_dev(dev_no, &dev));
	
	pthread_mutex_lock(&dev->lock);
	
	ret = av_stop(dev, AV_PLAY_FILE);
	
	pthread_mutex_unlock(&dev->lock);
	
	return ret;
}

/**\brief 暂停文件播放
 * \param dev_no 音视频设备号
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
AM_ErrorCode_t AM_AV_PauseFile(int dev_no)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	AM_TRY(av_get_openned_dev(dev_no, &dev));
	
	pthread_mutex_lock(&dev->lock);
	
	if(!(dev->mode&AV_PLAY_FILE))
	{
		AM_DEBUG(1, "do not in the file play mode");
		ret = AM_AV_ERR_ILLEGAL_OP;
	}
	
	if(ret==AM_SUCCESS)
	{
		if(dev->drv->file_cmd)
		{
			ret = dev->drv->file_cmd(dev, AV_PLAY_PAUSE, NULL);
		}
	}
	
	pthread_mutex_unlock(&dev->lock);
	
	return ret;
}

/**\brief 恢复文件播放
 * \param dev_no 音视频设备号
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
AM_ErrorCode_t AM_AV_ResumeFile(int dev_no)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	AM_TRY(av_get_openned_dev(dev_no, &dev));
	
	pthread_mutex_lock(&dev->lock);
	
	if(!(dev->mode&AV_PLAY_FILE))
	{
		AM_DEBUG(1, "do not in the file play mode");
		ret = AM_AV_ERR_ILLEGAL_OP;
	}
	
	if(ret==AM_SUCCESS)
	{
		if(dev->drv->file_cmd)
		{
			ret = dev->drv->file_cmd(dev, AV_PLAY_RESUME, NULL);
		}
	}
	
	pthread_mutex_unlock(&dev->lock);
	
	return ret;
}

/**\brief 设定当前文件播放位置
 * \param dev_no 音视频设备号
 * \param pos 播放位置
 * \param start 是否开始播放
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
AM_ErrorCode_t AM_AV_SeekFile(int dev_no, int pos, AM_Bool_t start)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	AV_FileSeekPara_t para;
	
	AM_TRY(av_get_openned_dev(dev_no, &dev));
	
	pthread_mutex_lock(&dev->lock);
	
	if(!(dev->mode&AV_PLAY_FILE))
	{
		AM_DEBUG(1, "do not in the file play mode");
		ret = AM_AV_ERR_ILLEGAL_OP;
	}
	
	if(ret==AM_SUCCESS)
	{
		if(dev->drv->file_cmd)
		{
			para.pos   = pos;
			para.start = start;
			ret = dev->drv->file_cmd(dev, AV_PLAY_SEEK, &para);
		}
	}
	
	pthread_mutex_unlock(&dev->lock);
	
	return ret;
}

/**\brief 快速向前播放
 * \param dev_no 音视频设备号
 * \param speed 播放速度
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
AM_ErrorCode_t AM_AV_FastForwardFile(int dev_no, int speed)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	if(speed<0)
	{
		AM_DEBUG(1, "speed must >= 0");
		return AM_AV_ERR_INVAL_ARG;
	}

	AM_TRY(av_get_openned_dev(dev_no, &dev));
	
	pthread_mutex_lock(&dev->lock);
	
	if(!(dev->mode&AV_PLAY_FILE))
	{
		AM_DEBUG(1, "do not in the file play mode");
		ret = AM_AV_ERR_ILLEGAL_OP;
	}
	
	if(ret==AM_SUCCESS)
	{
		if(dev->drv->file_cmd && (dev->file_player.speed!=speed))
		{
			ret = dev->drv->file_cmd(dev, AV_PLAY_FF, (void*)speed);
			if(ret==AM_SUCCESS)
			{
				dev->file_player.speed = speed;
				AM_EVT_Signal(dev->dev_no, AM_AV_EVT_PLAYER_SPEED_CHANGED, (void*)speed);
			}
		}
	}
	
	pthread_mutex_unlock(&dev->lock);
	
	return ret;
}

/**\brief 快速向后播放
 * \param dev_no 音视频设备号
 * \param speed 播放速度
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
AM_ErrorCode_t AM_AV_FastBackwardFile(int dev_no, int speed)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	if(speed<0)
	{
		AM_DEBUG(1, "speed must >= 0");
		return AM_AV_ERR_INVAL_ARG;
	}
	
	AM_TRY(av_get_openned_dev(dev_no, &dev));
	
	pthread_mutex_lock(&dev->lock);
	
	if(!(dev->mode&AV_PLAY_FILE))
	{
		AM_DEBUG(1, "do not in the file play mode");
		ret = AM_AV_ERR_ILLEGAL_OP;
	}
	
	if(ret==AM_SUCCESS)
	{
		if(dev->drv->file_cmd && (dev->file_player.speed!=-speed))
		{
			ret = dev->drv->file_cmd(dev, AV_PLAY_FB, (void*)speed);
			if(ret==AM_SUCCESS)
			{
				dev->file_player.speed = -speed;
				AM_EVT_Signal(dev->dev_no, AM_AV_EVT_PLAYER_SPEED_CHANGED, (void*)-speed);
			}
		}
	}
	
	pthread_mutex_unlock(&dev->lock);
	
	return ret;
}

/**\brief 取得当前媒体文件信息
 * \param dev_no 音视频设备号
 * \param[out] info 返回媒体文件信息
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
AM_ErrorCode_t AM_AV_GetCurrFileInfo(int dev_no, AM_AV_FileInfo_t *info)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	assert(info);
	
	AM_TRY(av_get_openned_dev(dev_no, &dev));
	
	pthread_mutex_lock(&dev->lock);
	
	if(!(dev->mode&AV_PLAY_FILE))
	{
		AM_DEBUG(1, "do not in the file play mode");
		ret = AM_AV_ERR_ILLEGAL_OP;
	}
	
	if(ret==AM_SUCCESS)
	{
		if(dev->drv->file_info)
		{
			ret = dev->drv->file_info(dev, info);
		}
	}
	
	pthread_mutex_unlock(&dev->lock);
	
	return ret;
}

/**\brief 取得媒体文件播放状态
 * \param dev_no 音视频设备号
 * \param[out] status 返回媒体文件播放状态
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
AM_ErrorCode_t AM_AV_GetPlayStatus(int dev_no, AM_AV_PlayStatus_t *status)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	assert(status);
	
	AM_TRY(av_get_openned_dev(dev_no, &dev));
	
	pthread_mutex_lock(&dev->lock);
	
	if(!(dev->mode&AV_PLAY_FILE))
	{
		AM_DEBUG(1, "do not in the file play mode");
		ret = AM_AV_ERR_ILLEGAL_OP;
	}
	
	if(ret==AM_SUCCESS)
	{
		if(!dev->drv->file_status)
		{
			AM_DEBUG(1, "do not support file_status");
			ret = AM_AV_ERR_NOT_SUPPORTED;
		}
	}
	
	if(ret==AM_SUCCESS)
	{
		ret = dev->drv->file_status(dev, status);
	}
	
	pthread_mutex_unlock(&dev->lock);
	
	return ret;
}

/**\brief 取得JPEG图片文件属性
 * \param dev_no 音视频设备号
 * \param[in] fname 图片文件名
 * \param[out] info 文件属性
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
AM_ErrorCode_t AM_AV_GetJPEGInfo(int dev_no, const char *fname, AM_AV_JPEGInfo_t *info)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	uint8_t *data;
	struct stat sbuf;
	int len;
	int fd;
	AV_DataPlayPara_t para;
	
	assert(fname && info);
	
	AM_TRY(av_get_openned_dev(dev_no, &dev));
	
	fd = open(fname, O_RDONLY);
	if(fd==-1)
	{
		AM_DEBUG(1, "cannot open \"%s\"", fname);
		return AM_AV_ERR_CANNOT_OPEN_FILE;
	}
	
	if(stat(fname, &sbuf)==-1)
	{
		AM_DEBUG(1, "stat failed");
		close(fd);
		return AM_AV_ERR_SYS;
	}
	
	len = sbuf.st_size;
	
	data = mmap(NULL, len, PROT_READ, MAP_SHARED, fd, 0);
	if(data==(void*)-1)
	{
		AM_DEBUG(1, "mmap failed");
		close(fd);
		return AM_AV_ERR_SYS;
	}
	
	para.data  = data;
	para.len   = len;
	para.times = 1;
	para.fd    = fd;
	para.need_free = AM_TRUE;
	para.para  = info;
	
	pthread_mutex_lock(&dev->lock);
	
	ret = av_start(dev, AV_GET_JPEG_INFO, &para);
	
	pthread_mutex_unlock(&dev->lock);
	
	if(ret!=AM_SUCCESS)
		av_free_data_para(&para);
	
	return ret;
}

/**\brief 取得JPEG图片数据属性
 * \param dev_no 音视频设备号
 * \param[in] data 图片数据缓冲区
 * \param len 数据缓冲区大小
 * \param[out] info 文件属性
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
AM_ErrorCode_t AM_AV_GetJPEGDataInfo(int dev_no, const uint8_t *data, int len, AM_AV_JPEGInfo_t *info)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	AV_DataPlayPara_t para;
	
	assert(data && info);
	
	AM_TRY(av_get_openned_dev(dev_no, &dev));
	
	para.data  = data;
	para.len   = len;
	para.times = 1;
	para.fd    = -1;
	para.need_free = AM_FALSE;
	para.para  = info;
	
	pthread_mutex_lock(&dev->lock);
	
	ret = av_start(dev, AV_GET_JPEG_INFO, info);
	
	pthread_mutex_unlock(&dev->lock);
	
	if(ret!=AM_SUCCESS)
		av_free_data_para(&para);
	
	return ret;
}

/**\brief 解码JPEG图片文件
 * \param dev_no 音视频设备号
 * \param[in] fname 图片文件名
 * \param angle 图片旋转角度
 * \param[out] surf 返回JPEG图片的surface
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
AM_ErrorCode_t AM_AV_DecodeJPEG(int dev_no, const char *fname, AM_AV_JPEGAngle_t angle, AM_OSD_Surface_t **surf)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	uint8_t *data;
	struct stat sbuf;
	int len;
	int fd;
	AV_DataPlayPara_t para;
	AV_JPEGDecodePara_t jpeg;
	
	assert(fname && surf);
	
	AM_TRY(av_get_openned_dev(dev_no, &dev));
	
	fd = open(fname, O_RDONLY);
	if(fd==-1)
	{
		AM_DEBUG(1, "cannot open \"%s\"", fname);
		return AM_AV_ERR_CANNOT_OPEN_FILE;
	}
	
	if(stat(fname, &sbuf)==-1)
	{
		AM_DEBUG(1, "stat failed");
		close(fd);
		return AM_AV_ERR_SYS;
	}
	
	len = sbuf.st_size;
	
	data = mmap(NULL, len, PROT_READ, MAP_SHARED, fd, 0);
	if(data==(void*)-1)
	{
		AM_DEBUG(1, "mmap failed");
		close(fd);
		return AM_AV_ERR_SYS;
	}
	
	para.data  = data;
	para.len   = len;
	para.times = 1;
	para.fd    = fd;
	para.need_free = AM_TRUE;
	para.para  = &jpeg;
	
	jpeg.surface = NULL;
	jpeg.angle   = angle;
	
	pthread_mutex_lock(&dev->lock);
	
	ret = av_start(dev, AV_DECODE_JPEG, &para);
	
	pthread_mutex_unlock(&dev->lock);
	
	if(ret!=AM_SUCCESS)
		av_free_data_para(&para);
	else
		*surf = jpeg.surface;
	
	return ret;
}

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
AM_ErrorCode_t AM_AV_DacodeJPEGData(int dev_no, const uint8_t *data, int len, AM_AV_JPEGAngle_t angle, AM_OSD_Surface_t **surf)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	AV_DataPlayPara_t para;
	AV_JPEGDecodePara_t jpeg;
	
	assert(data && surf);
	
	AM_TRY(av_get_openned_dev(dev_no, &dev));
	
	para.data  = data;
	para.len   = len;
	para.times = 1;
	para.fd    = -1;
	para.need_free = AM_FALSE;
	para.para  = &jpeg;
	
	jpeg.surface = NULL;
	jpeg.angle   = angle;
	
	pthread_mutex_lock(&dev->lock);
	
	ret = av_start(dev, AV_DECODE_JPEG, &para);
	
	pthread_mutex_unlock(&dev->lock);
	
	if(ret!=AM_SUCCESS)
		av_free_data_para(&para);
	else
		*surf = jpeg.surface;
	
	return ret;
}

/**\brief 解码视频基础流文件
 * \param dev_no 音视频设备号
 * \param format 视频压缩格式
 * \param[in] fname 视频文件名
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
AM_ErrorCode_t AM_AV_StartVideoES(int dev_no, vformat_t format, const char *fname)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	uint8_t *data;
	struct stat sbuf;
	int len;
	int fd;
	AV_DataPlayPara_t para;
	
	assert(fname);
	
	AM_TRY(av_get_openned_dev(dev_no, &dev));
	
	fd = open(fname, O_RDONLY);
	if(fd==-1)
	{
		AM_DEBUG(1, "cannot open \"%s\"", fname);
		return AM_AV_ERR_CANNOT_OPEN_FILE;
	}
	
	if(stat(fname, &sbuf)==-1)
	{
		AM_DEBUG(1, "stat failed");
		close(fd);
		return AM_AV_ERR_SYS;
	}
	
	len = sbuf.st_size;
	
	data = mmap(NULL, len, PROT_READ, MAP_SHARED, fd, 0);
	if(data==(void*)-1)
	{
		AM_DEBUG(1, "mmap failed");
		close(fd);
		return AM_AV_ERR_SYS;
	}
	
	para.data  = data;
	para.len   = len;
	para.times = 1;
	para.fd    = fd;
	para.need_free = AM_TRUE;
	para.para  = (void*)format;
	
	pthread_mutex_lock(&dev->lock);
	
	ret = av_start(dev, AV_PLAY_VIDEO_ES, &para);
	
	pthread_mutex_unlock(&dev->lock);
	
	if(ret!=AM_SUCCESS)
		av_free_data_para(&para);
	
	return ret;
}

/**\brief 解码视频基础流数据
 * \param dev_no 音视频设备号
 * \param format 视频压缩格式
 * \param[in] data 视频数据缓冲区
 * \param len 数据缓冲区大小
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
AM_ErrorCode_t AM_AV_StartVideoESData(int dev_no, vformat_t format, const uint8_t *data, int len)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	AV_DataPlayPara_t para;
	
	assert(data);
	
	AM_TRY(av_get_openned_dev(dev_no, &dev));
	
	para.data  = data;
	para.len   = len;
	para.times = 1;
	para.fd    = -1;
	para.need_free = AM_FALSE;
	para.para  = (void*)format;
	
	pthread_mutex_lock(&dev->lock);
	
	ret = av_start(dev, AV_PLAY_VIDEO_ES, &para);
	
	pthread_mutex_unlock(&dev->lock);
	
	if(ret!=AM_SUCCESS)
		av_free_data_para(&para);
	
	return ret;
}

/**\brief 解码音频基础流文件
 * \param dev_no 音视频设备号
 * \param format 音频压缩格式
 * \param[in] fname 视频文件名
 * \param times 播放次数，<=0表示一直播放
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
AM_ErrorCode_t AM_AV_StartAudioES(int dev_no, aformat_t format, const char *fname, int times)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	uint8_t *data;
	struct stat sbuf;
	int len;
	int fd;
	AV_DataPlayPara_t para;
	
	assert(fname);
	
	AM_TRY(av_get_openned_dev(dev_no, &dev));
	
	fd = open(fname, O_RDONLY);
	if(fd==-1)
	{
		AM_DEBUG(1, "cannot open \"%s\"", fname);
		return AM_AV_ERR_CANNOT_OPEN_FILE;
	}
	
	if(stat(fname, &sbuf)==-1)
	{
		AM_DEBUG(1, "stat failed");
		close(fd);
		return AM_AV_ERR_SYS;
	}
	
	len = sbuf.st_size;
	
	data = mmap(NULL, len, PROT_READ, MAP_SHARED, fd, 0);
	if(data==(void*)-1)
	{
		AM_DEBUG(1, "mmap failed");
		close(fd);
		return AM_AV_ERR_SYS;
	}
	
	para.data  = data;
	para.len   = len;
	para.times = times;
	para.fd    = fd;
	para.need_free = AM_TRUE;
	para.para  = (void*)format;
	
	pthread_mutex_lock(&dev->lock);
	
	ret = av_start(dev, AV_PLAY_AUDIO_ES, &para);
	
	pthread_mutex_unlock(&dev->lock);
	
	if(ret!=AM_SUCCESS)
		av_free_data_para(&para);
	
	return ret;
}

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
AM_ErrorCode_t AM_AV_StartAudioESData(int dev_no, aformat_t format, const uint8_t *data, int len, int times)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	AV_DataPlayPara_t para;
	
	assert(data);
	
	AM_TRY(av_get_openned_dev(dev_no, &dev));
	
	para.data  = data;
	para.len   = len;
	para.times = times;
	para.fd    = -1;
	para.need_free = AM_FALSE;
	para.para  = (void*)format;
	
	pthread_mutex_lock(&dev->lock);
	
	ret = av_start(dev, AV_PLAY_AUDIO_ES, &para);
	
	pthread_mutex_unlock(&dev->lock);
	
	if(ret!=AM_SUCCESS)
		av_free_data_para(&para);
	
	return ret;
}

/**\brief 停止解码音频基础流
 * \param dev_no 音视频设备号
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
AM_ErrorCode_t AM_AV_StopAudioES(int dev_no)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	AM_TRY(av_get_openned_dev(dev_no, &dev));
	
	pthread_mutex_lock(&dev->lock);
	
	ret = av_stop(dev, AV_PLAY_AUDIO_ES);
	
	pthread_mutex_unlock(&dev->lock);
	
	return ret;
}

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
AM_ErrorCode_t AM_AV_SetVideoWindow(int dev_no, int x, int y, int w, int h)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	AV_VideoWindow_t win;
	
	AM_TRY(av_get_openned_dev(dev_no, &dev));
	
	win.x = x;
	win.y = y;
	win.w = w;
	win.h = h;
	
	pthread_mutex_lock(&dev->lock);
	
	if((dev->video_x!=x) || (dev->video_y!=y) || (dev->video_w!=w) || (dev->video_h!=h))
	{
		if(dev->drv->set_video_para)
		{
			ret = dev->drv->set_video_para(dev, AV_VIDEO_PARA_WINDOW, &win);
		}
	
		if(ret==AM_SUCCESS)
		{
			AM_AV_VideoWindow_t win;
			
			dev->video_x = x;
			dev->video_y = y;
			dev->video_w = w;
			dev->video_h = h;
			
			win.x = x;
			win.y = y;
			win.w = w;
			win.h = h;
			
			AM_EVT_Signal(dev->dev_no, AM_AV_EVT_VIDEO_WINDOW_CHANGED, &win);
		}
	}
	
	pthread_mutex_unlock(&dev->lock);
	
	return ret;
}

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
AM_ErrorCode_t AM_AV_GetVideoWindow(int dev_no, int *x, int *y, int *w, int *h)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	AM_TRY(av_get_openned_dev(dev_no, &dev));
	
	pthread_mutex_lock(&dev->lock);
	
	if(x)
		*x = dev->video_x;
	if(y)
		*y = dev->video_y;
	if(w)
		*w = dev->video_w;
	if(h)
		*h = dev->video_h;
	
	pthread_mutex_unlock(&dev->lock);
	
	return ret;
}

/**\brief 设定视频对比度(0~100)
 * \param dev_no 音视频设备号
 * \param val 视频对比度值
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
AM_ErrorCode_t AM_AV_SetVideoContrast(int dev_no, int val)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	AM_TRY(av_get_openned_dev(dev_no, &dev));
	
	pthread_mutex_lock(&dev->lock);
	
	if(dev->video_contrast!=val)
	{
		if(dev->drv->set_video_para)
		{
			ret = dev->drv->set_video_para(dev, AV_VIDEO_PARA_CONTRAST, (void*)val);
		}
	
		if(ret==AM_SUCCESS)
		{
			dev->video_contrast = val;
			AM_EVT_Signal(dev->dev_no, AM_AV_EVT_VIDEO_CONTRAST_CHANGED, (void*)val);
		}
	}
	
	pthread_mutex_unlock(&dev->lock);
	
	return ret;
}

/**\brief 取得当前视频对比度
 * \param dev_no 音视频设备号
 * \param[out] val 返回当前视频对比度值
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
AM_ErrorCode_t AM_AV_GetVideoContrast(int dev_no, int *val)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	AM_TRY(av_get_openned_dev(dev_no, &dev));
	
	pthread_mutex_lock(&dev->lock);
	
	if(val)
		*val = dev->video_contrast;
	
	pthread_mutex_unlock(&dev->lock);
	
	return ret;
}

/**\brief 设定视频饱和度(0~100)
 * \param dev_no 音视频设备号
 * \param val 视频饱和度值
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
AM_ErrorCode_t AM_AV_SetVideoSaturation(int dev_no, int val)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	AM_TRY(av_get_openned_dev(dev_no, &dev));
	
	pthread_mutex_lock(&dev->lock);
	
	if(dev->video_saturation!=val)
	{
		if(dev->drv->set_video_para)
		{
			ret = dev->drv->set_video_para(dev, AV_VIDEO_PARA_SATURATION, (void*)val);
		}
	
		if(ret==AM_SUCCESS)
		{
			dev->video_saturation = val;
			AM_EVT_Signal(dev->dev_no, AM_AV_EVT_VIDEO_SATURATION_CHANGED, (void*)val);
		}
	}
	
	pthread_mutex_unlock(&dev->lock);
	
	return ret;
}

/**\brief 取得当前视频饱和度
 * \param dev_no 音视频设备号
 * \param[out] val 返回当前视频饱和度值
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
AM_ErrorCode_t AM_AV_GetVideoSaturation(int dev_no, int *val)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	AM_TRY(av_get_openned_dev(dev_no, &dev));
	
	pthread_mutex_lock(&dev->lock);
	
	if(val)
		*val = dev->video_saturation;
	
	pthread_mutex_unlock(&dev->lock);
	
	return ret;
}

/**\brief 设定视频亮度(0~100)
 * \param dev_no 音视频设备号
 * \param val 视频亮度值
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
AM_ErrorCode_t AM_AV_SetVideoBrightness(int dev_no, int val)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	AM_TRY(av_get_openned_dev(dev_no, &dev));
	
	pthread_mutex_lock(&dev->lock);
	
	if(dev->video_brightness!=val)
	{
		if(dev->drv->set_video_para)
		{
			ret = dev->drv->set_video_para(dev, AV_VIDEO_PARA_BRIGHTNESS, (void*)val);
		}
	
		if(ret==AM_SUCCESS)
		{
			dev->video_brightness = val;
			AM_EVT_Signal(dev->dev_no, AM_AV_EVT_VIDEO_BRIGHTNESS_CHANGED, (void*)val);
		}
	}
	
	pthread_mutex_unlock(&dev->lock);
	
	return ret;
}

/**\brief 取得当前视频亮度
 * \param dev_no 音视频设备号
 * \param[out] val 返回当前视频亮度值
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
AM_ErrorCode_t AM_AV_GetVideoBrightness(int dev_no, int *val)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	AM_TRY(av_get_openned_dev(dev_no, &dev));
	
	pthread_mutex_lock(&dev->lock);
	
	if(val)
		*val = dev->video_saturation;
	
	pthread_mutex_unlock(&dev->lock);
	
	return ret;
}

/**\brief 显示视频层
 * \param dev_no 音视频设备号
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
AM_ErrorCode_t AM_AV_EnableVideo(int dev_no)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	AM_TRY(av_get_openned_dev(dev_no, &dev));
	
	pthread_mutex_lock(&dev->lock);
	
	if(!dev->video_enable)
	{
		if(dev->drv->set_video_para)
		{
			ret = dev->drv->set_video_para(dev, AV_VIDEO_PARA_ENABLE, (void*)AM_TRUE);
		}

		if(ret==AM_SUCCESS)
		{
			dev->video_enable = AM_TRUE;
			AM_EVT_Signal(dev->dev_no, AM_AV_EVT_VIDEO_ENABLED, NULL);
		}
	}
	
	pthread_mutex_unlock(&dev->lock);
	
	return ret;
}

/**\brief 隐藏视频层
 * \param dev_no 音视频设备号
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
AM_ErrorCode_t AM_AV_DisableVideo(int dev_no)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	AM_TRY(av_get_openned_dev(dev_no, &dev));
	
	pthread_mutex_lock(&dev->lock);
	
	if(dev->video_enable)
	{
		if(dev->drv->set_video_para)
		{
			ret = dev->drv->set_video_para(dev, AV_VIDEO_PARA_ENABLE, (void*)AM_FALSE);
		}

		if(ret==AM_SUCCESS)
		{
			dev->video_enable = AM_FALSE;
			AM_EVT_Signal(dev->dev_no, AM_AV_EVT_VIDEO_DISABLED, NULL);
		}
	}
	
	pthread_mutex_unlock(&dev->lock);
	
	return ret;
}

/**\brief 设定视频长宽比
 * \param dev_no 音视频设备号
 * \param ratio 长宽比
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
AM_ErrorCode_t AM_AV_SetVideoAspectRatio(int dev_no, AM_AV_VideoAspectRatio_t ratio)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	AM_TRY(av_get_openned_dev(dev_no, &dev));
	
	pthread_mutex_lock(&dev->lock);
	
	if(dev->drv->set_video_para)
	{
		ret = dev->drv->set_video_para(dev, AV_VIDEO_PARA_RATIO, (void*)ratio);
	}
	
	if(ret==AM_SUCCESS)
	{
		dev->video_ratio = ratio;
		AM_EVT_Signal(dev->dev_no, AM_AV_EVT_VIDEO_ASPECT_RATIO_CHANGED, (void*)ratio);
	}
	
	pthread_mutex_unlock(&dev->lock);
	
	return ret;
}

/**\brief 取得视频长宽比
 * \param dev_no 音视频设备号
 * \param[out] ratio 长宽比
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
AM_ErrorCode_t AM_AV_GetVideoAspectRatio(int dev_no, AM_AV_VideoAspectRatio_t *ratio)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	AM_TRY(av_get_openned_dev(dev_no, &dev));
	
	pthread_mutex_lock(&dev->lock);
	
	if(ratio)
		*ratio = dev->video_ratio;
	
	pthread_mutex_unlock(&dev->lock);
	
	return ret;
}

/**\brief 设定视频停止时自动隐藏视频层
 * \param dev_no 音视频设备号
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
AM_ErrorCode_t AM_AV_EnableVideoBlackout(int dev_no)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	AM_TRY(av_get_openned_dev(dev_no, &dev));
	
	pthread_mutex_lock(&dev->lock);
	
	if(dev->drv->set_video_para)
	{
		ret = dev->drv->set_video_para(dev, AV_VIDEO_PARA_BLACKOUT, (void*)AM_TRUE);
	}

	if(ret==AM_SUCCESS)
	{
		dev->video_blackout = AM_TRUE;
	}
	
	pthread_mutex_unlock(&dev->lock);
	
	return ret;
}

/**\brief 视频停止时不隐藏视频层
 * \param dev_no 音视频设备号
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
AM_ErrorCode_t AM_AV_DisableVideoBlackout(int dev_no)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	AM_TRY(av_get_openned_dev(dev_no, &dev));
	
	pthread_mutex_lock(&dev->lock);
	
	if(dev->drv->set_video_para)
	{
		ret = dev->drv->set_video_para(dev, AV_VIDEO_PARA_BLACKOUT, (void*)AM_FALSE);
	}

	if(ret==AM_SUCCESS)
	{
		dev->video_blackout = AM_FALSE;
	}
	
	pthread_mutex_unlock(&dev->lock);
	
	return ret;
}

/**\brief 设定视频显示模式
 * \param dev_no 音视频设备号
 * \param mode 视频显示模式
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
AM_ErrorCode_t AM_AV_SetVideoDisplayMode(int dev_no, AM_AV_VideoDisplayMode_t mode)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	AM_TRY(av_get_openned_dev(dev_no, &dev));
	
	pthread_mutex_lock(&dev->lock);
	
	if(dev->video_mode!=mode)
	{
		if(dev->drv->set_video_para)
		{
			ret = dev->drv->set_video_para(dev, AV_VIDEO_PARA_MODE, (void*)mode);
		}
	
		if(ret==AM_SUCCESS)
		{
			dev->video_mode = mode;
			AM_EVT_Signal(dev->dev_no, AM_AV_EVT_VIDEO_DISPLAY_MODE_CHANGED, (void*)mode);
		}
	}
	
	pthread_mutex_unlock(&dev->lock);
	
	return ret;
}

/**\brief 取得当前视频显示模式
 * \param dev_no 音视频设备号
 * \param mode 返回当前视频显示模式
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
AM_ErrorCode_t AM_AV_GetVideoDisplayMode(int dev_no, AM_AV_VideoDisplayMode_t *mode)
{
	AM_AV_Device_t *dev;
	AM_ErrorCode_t ret = AM_SUCCESS;
	
	AM_TRY(av_get_openned_dev(dev_no, &dev));
	
	pthread_mutex_lock(&dev->lock);
	
	if(mode)
		*mode = dev->video_mode;
	
	pthread_mutex_unlock(&dev->lock);
	
	return ret;
}

/**\brief Set A/V sync mode
 * \param dev_no 音视频设备号
 * \param mode sync mode
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
AM_ErrorCode_t AM_AV_SetSyncMode(int dev_no, AM_AV_SyncMode_t mode)
{
  AM_AV_Device_t *dev;
  char *path = "/sys/class/tsync/enable";    
  char  bcmd[16];
  AM_ErrorCode_t ret = AM_SUCCESS;
  
  AM_TRY(av_get_openned_dev(dev_no, &dev));
  
  pthread_mutex_lock(&dev->lock);

  if(dev->sync_mode!=mode)
  {

    if(amsysfs_set_sysfs_str(path, bcmd) != -1)
    {
      sprintf(bcmd,"%d",(mode==AM_AV_SYNC_ENABLE));
      dev->sync_mode = mode;
      AM_EVT_Signal(dev->dev_no, AM_AV_EVT_SYNC_MODE_CHANGED, (void*)mode);
      if(dev->sync_mode==AM_AV_SYNC_ENABLE)
        log_print("A/V sync enabled\n");
      else
        log_print("A/V sync disabled\n");
    }
  }
  
  pthread_mutex_unlock(&dev->lock);
  
  return ret;
}

/**\brief Get A/V sync mode
 * \param dev_no 音视频设备号
 * \param mode sync mode
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
extern AM_ErrorCode_t AM_AV_GetSyncMode(int dev_no, AM_AV_SyncMode_t *mode)
{
  AM_AV_Device_t *dev;
  AM_ErrorCode_t ret = AM_SUCCESS;
  
  AM_TRY(av_get_openned_dev(dev_no, &dev));
  
  pthread_mutex_lock(&dev->lock);
  
  if(mode)
    *mode = dev->sync_mode;
  
  pthread_mutex_unlock(&dev->lock);
  
  return ret;
}

/**\brief Set PCR recover mode
 * \param dev_no 音视频设备号
 * \param mode PCR Recover mode
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
AM_ErrorCode_t AM_AV_SetPCRRecoverMode(int dev_no, AM_AV_PCRRecoverMode_t mode)
{
  AM_AV_Device_t *dev;
  int fd;
  char *path = "/sys/class/tsync/pcr_recover";
  char  bcmd[16];
  AM_ErrorCode_t ret = AM_SUCCESS;
  
  AM_TRY(av_get_openned_dev(dev_no, &dev));
  
  pthread_mutex_lock(&dev->lock);
  
  if(dev->recover_mode!=mode)
  {
    if(amsysfs_set_sysfs_str(path, bcmd) != -1)
    {
      sprintf(bcmd,"%d",(mode==AM_AV_RECOVER_ENABLE));
      dev->recover_mode = mode;
      AM_EVT_Signal(dev->dev_no, AM_AV_EVT_PCRRECOVER_MODE_CHANGED, (void*)mode);
      if(dev->recover_mode==AM_AV_RECOVER_ENABLE)
        log_print("PCR recover enabled\n");
      else
        log_print("PCR recover disabled\n");
    }
  }
  
  pthread_mutex_unlock(&dev->lock);
  
  return ret;
}

/**\brief Get PCR Recover mode
 * \param dev_no 音视频设备号
 * \param mode PCR Recover mode
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_av.h)
 */
extern AM_ErrorCode_t AM_AV_GetPCRRecoverMode(int dev_no, AM_AV_PCRRecoverMode_t *mode)
{
  AM_AV_Device_t *dev;
  AM_ErrorCode_t ret = AM_SUCCESS;
  
  AM_TRY(av_get_openned_dev(dev_no, &dev));
  
  pthread_mutex_lock(&dev->lock);
  
  if(mode)
    *mode = dev->recover_mode;
  
  pthread_mutex_unlock(&dev->lock);
  
  return ret;
}