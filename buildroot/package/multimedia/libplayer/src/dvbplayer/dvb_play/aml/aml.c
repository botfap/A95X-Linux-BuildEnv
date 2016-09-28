/***************************************************************************
 *  Copyright C 2009 by Amlogic, Inc. All Rights Reserved.
 */
/**\file
 * \brief AMLogic 音视频解码驱动
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-08-09: create the document
 ***************************************************************************/

#define AM_DEBUG_LEVEL 5

#include <am_debug.h>
#include <am_misc.h>
#include <am_mem.h>
#include <am_evt.h>
#include <am_thread.h>
#include "../am_av_internal.h"
#include "../am_aout_internal.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <errno.h>
#include <amports/jpegdec.h>
#include <log_print.h>
#include "player.h"
#include <audio_priv.h>

/****************************************************************************
 * Macro definitions
 ***************************************************************************/
#define AOUT_DEV_NO 0

static void * adec_priv;
static int has_audio = 0;

#define STREAM_VBUF_FILE    "/dev/amstream_vbuf"
#define STREAM_ABUF_FILE    "/dev/amstream_abuf"
#define STREAM_TS_FILE      "/dev/amstream_mpts"
#define JPEG_DEC_FILE       "/dev/amjpegdec"

#define VID_AXIS_FILE       "/sys/class/video/axis"
#define VID_CONTRAST_FILE   "/sys/class/video/contrast"
#define VID_SATURATION_FILE "/sys/class/video/saturation"
#define VID_BRIGHTNESS_FILE "/sys/class/video/brightness"
#define VID_DISABLE_FILE    "/sys/class/video/disable_video"
#define VID_BLACKOUT_FILE   "/sys/class/video/blackout_policy"
#define VID_SCREEN_MODE_FILE  "/sys/class/video/screen_mode"

#define DISP_MODE_FILE      "/sys/class/display/mode"

#define CANVAS_ALIGN(x)    (((x)+7)&~7)
#define JPEG_WRTIE_UNIT    (32*1024)

/****************************************************************************
 * Type definitions
 ***************************************************************************/

/**\brief 音视频数据注入*/
typedef struct
{
	int             fd;
	pthread_t       thread;
	pthread_mutex_t lock;
	pthread_cond_t  cond;
	const uint8_t  *data;
	int             len;
	int             times;
	int             pos;
	AM_Bool_t       running;
} AV_DataSource_t;

/**\brief JPEG解码相关数据*/
typedef struct
{
	int             vbuf_fd;
	int             dec_fd;
} AV_JPEGData_t;

/**\brief JPEG解码器状态*/
typedef enum {
	AV_JPEG_DEC_STAT_DECDEV,
	AV_JPEG_DEC_STAT_INFOCONFIG,
	AV_JPEG_DEC_STAT_INFO,
	AV_JPEG_DEC_STAT_DECCONFIG,
	AV_JPEG_DEC_STAT_RUN
} AV_JPEGDecState_t;

/**\brief 文件播放器应答类型*/
typedef enum
{
	AV_MP_RESP_OK,
	AV_MP_RESP_ERR,
	AV_MP_RESP_STOP,
	AV_MP_RESP_DURATION,
	AV_MP_RESP_POSITION,
	AV_MP_RESP_MEDIA
} AV_MpRespType_t;

/**\brief 文件播放器数据*/
typedef struct
{
#if 0
	int                media_id;
	AV_MpRespType_t    resp_type;
	union
	{
		int        duration;
		int        position;
		MP_AVMediaFileInfo media;
	} resp_data;
	pthread_mutex_t    lock;
	pthread_cond_t     cond;
	MP_PlayBackState   state;
	AM_AV_Device_t    *av;
#endif
} AV_FilePlayerData_t;

/****************************************************************************
 * Static data
 ***************************************************************************/

/*音视频设备操作*/
static AM_ErrorCode_t aml_open(AM_AV_Device_t *dev, const AM_AV_OpenPara_t *para);
static AM_ErrorCode_t aml_close(AM_AV_Device_t *dev);
static AM_ErrorCode_t aml_open_mode(AM_AV_Device_t *dev, AV_PlayMode_t mode);
static AM_ErrorCode_t aml_start_mode(AM_AV_Device_t *dev, AV_PlayMode_t mode, void *para);
static AM_ErrorCode_t aml_close_mode(AM_AV_Device_t *dev, AV_PlayMode_t mode);
static AM_ErrorCode_t aml_ts_source(AM_AV_Device_t *dev, AM_AV_TSSource_t src);
static AM_ErrorCode_t aml_file_cmd(AM_AV_Device_t *dev, AV_PlayCmd_t cmd, void *data);
static AM_ErrorCode_t aml_file_status(AM_AV_Device_t *dev, AM_AV_PlayStatus_t *status);
static AM_ErrorCode_t aml_file_info(AM_AV_Device_t *dev, AM_AV_FileInfo_t *info);
static AM_ErrorCode_t aml_set_video_para(AM_AV_Device_t *dev, AV_VideoParaType_t para, void *val);

const AM_AV_Driver_t aml_av_drv =
{
.open        = aml_open,
.close       = aml_close,
.open_mode   = aml_open_mode,
.start_mode  = aml_start_mode,
.close_mode  = aml_close_mode,
.ts_source   = aml_ts_source,
.file_cmd    = aml_file_cmd,
.file_status = aml_file_status,
.file_info   = aml_file_info,
.set_video_para = aml_set_video_para
};

/*音频控制（通过解码器）操作*/
static AM_ErrorCode_t adec_open(AM_AOUT_Device_t *dev, const AM_AOUT_OpenPara_t *para);
static AM_ErrorCode_t adec_set_volume(AM_AOUT_Device_t *dev, int vol);
static AM_ErrorCode_t adec_set_mute(AM_AOUT_Device_t *dev, AM_Bool_t mute);
static AM_ErrorCode_t adec_set_output_mode(AM_AOUT_Device_t *dev, AM_AOUT_OutputMode_t mode);
static AM_ErrorCode_t adec_close(AM_AOUT_Device_t *dev);

const AM_AOUT_Driver_t adec_aout_drv =
{
.open         = adec_open,
.set_volume   = adec_set_volume,
.set_mute     = adec_set_mute,
.set_output_mode = adec_set_output_mode,
.close        = adec_close
};

/*音频控制（通过amplayer2）操作*/
static AM_ErrorCode_t amp_open(AM_AOUT_Device_t *dev, const AM_AOUT_OpenPara_t *para);
static AM_ErrorCode_t amp_set_volume(AM_AOUT_Device_t *dev, int vol);
static AM_ErrorCode_t amp_set_mute(AM_AOUT_Device_t *dev, AM_Bool_t mute);
static AM_ErrorCode_t amp_set_output_mode(AM_AOUT_Device_t *dev, AM_AOUT_OutputMode_t mode);
static AM_ErrorCode_t amp_close(AM_AOUT_Device_t *dev);

const AM_AOUT_Driver_t amplayer_aout_drv =
{
.open         = amp_open,
.set_volume   = amp_set_volume,
.set_mute     = amp_set_mute,
.set_output_mode = amp_set_output_mode,
.close        = amp_close
};

/****************************************************************************
 * Static functions
 ***************************************************************************/

/*音频控制（通过解码器）操作*/

static AM_ErrorCode_t adec_cmd(const char *cmd)
{
#if 0
	AM_ErrorCode_t ret;
	char buf[32];
	int fd;
	
	ret = AM_LocalConnect("/tmp/amadec_socket", &fd);
	if(ret!=AM_SUCCESS)
		return ret;
	
	ret = AM_LocalSendCmd(fd, cmd);
	
	if(ret==AM_SUCCESS)
	{
		ret = AM_LocalGetResp(fd, buf, sizeof(buf));
	}
	
	close(fd);
	
	return ret;
#endif
	return 0;
}

static AM_ErrorCode_t adec_open(AM_AOUT_Device_t *dev, const AM_AOUT_OpenPara_t *para)
{
	return AM_SUCCESS;
}

static AM_ErrorCode_t adec_set_volume(AM_AOUT_Device_t *dev, int vol)
{
#if 0
	char buf[32];
	
	snprintf(buf, sizeof(buf), "volset:%d", vol);
	
	return adec_cmd(buf);
#endif
	return 0;
}

static AM_ErrorCode_t adec_set_mute(AM_AOUT_Device_t *dev, AM_Bool_t mute)
{
#if 0
	const char *cmd = mute?"mute":"unmute";
	
	return adec_cmd(cmd);
#endif
	return 0;

}

static AM_ErrorCode_t adec_set_output_mode(AM_AOUT_Device_t *dev, AM_AOUT_OutputMode_t mode)
{
	const char *cmd = NULL;
#if 0	
	switch(mode)
	{
		case AM_AOUT_OUTPUT_STEREO:
		default:
			cmd = "stereo";
		break;
		case AM_AOUT_OUTPUT_DUAL_LEFT:
			cmd = "leftmono";
		break;
		case AM_AOUT_OUTPUT_DUAL_RIGHT:
			cmd = "rightmono";
		break;
		case AM_AOUT_OUTPUT_SWAP:
			cmd = "swap";
		break;
	}
	
	return adec_cmd(cmd);
#endif
	return 0;
}

static AM_ErrorCode_t adec_close(AM_AOUT_Device_t *dev)
{
	return AM_SUCCESS;
}

/*音频控制（通过amplayer2）操作*/
static AM_ErrorCode_t amp_open(AM_AOUT_Device_t *dev, const AM_AOUT_OpenPara_t *para)
{
	return AM_SUCCESS;
}

static AM_ErrorCode_t amp_set_volume(AM_AOUT_Device_t *dev, int vol)
{
	int media_id = (int)dev->drv_data;
#if 0	
	if(MP_SetVolume(media_id, vol)==-1)
	{
		AM_DEBUG(1, "MP_SetVolume failed");
		return AM_AV_ERR_SYS;
	}
#endif	
	return AM_SUCCESS;
}

static AM_ErrorCode_t amp_set_mute(AM_AOUT_Device_t *dev, AM_Bool_t mute)
{
#if 0
	int media_id = (int)dev->drv_data;
	
	if(MP_mute(media_id, mute)==-1)
	{
		AM_DEBUG(1, "MP_SetVolume failed");
		return AM_AV_ERR_SYS;
	}
#endif	
	return AM_SUCCESS;
}

static AM_ErrorCode_t amp_set_output_mode(AM_AOUT_Device_t *dev, AM_AOUT_OutputMode_t mode)
{
#if 0
	int media_id = (int)dev->drv_data;
	MP_Tone tone;
	
	switch(mode)
	{
		case AM_AOUT_OUTPUT_STEREO:
		default:
			tone = MP_TONE_STEREO;
		break;
		case AM_AOUT_OUTPUT_DUAL_LEFT:
			tone = MP_TONE_LEFTMONO;
		break;
		case AM_AOUT_OUTPUT_DUAL_RIGHT:
			tone = MP_TONE_RIGHTMONO;
		break;
		case AM_AOUT_OUTPUT_SWAP:
			tone = MP_TONE_SWAP;
		break;
	}
	
	if(MP_SetTone(media_id, tone)==-1)
	{
		AM_DEBUG(1, "MP_SetTone failed");
		return AM_AV_ERR_SYS;
	}
#endif	
	return AM_SUCCESS;
}

static AM_ErrorCode_t amp_close(AM_AOUT_Device_t *dev)
{
	return AM_SUCCESS;
}

/**\brief 音视频数据注入线程*/
static void* aml_data_source_thread(void *arg)
{
	AV_DataSource_t *src = (AV_DataSource_t*)arg;
	struct pollfd pfd;
	struct timespec ts;
	int cnt;
#if 0
	while(src->running)
	{
		pthread_mutex_lock(&src->lock);
		
		if(src->pos>=0)
		{
			pfd.fd     = src->fd;
			pfd.events = POLLOUT;
			
			if(poll(&pfd, 1, 20)==1)
			{
				cnt = src->len-src->pos;
				cnt = write(src->fd, src->data+src->pos, cnt);
				if(cnt<=0)
				{
					AM_DEBUG(1, "write data failed");
				}
				else
				{
					src->pos += cnt;
				}
			}
		
			if(src->pos==src->len)
			{
				if(src->times>0)
				{
					src->times--;
					if(src->times)
					{
						src->pos = 0;
					}
					else
					{
						src->pos = -1;
					}
				}
			}
			
			AM_TIME_GetTimeSpecTimeout(20, &ts);
			pthread_cond_timedwait(&src->cond, &src->lock, &ts);
		}
		else
		{
			pthread_cond_wait(&src->cond, &src->lock);
		}
		
		pthread_mutex_unlock(&src->lock);
		
	}
#endif 	
	return NULL;
}

/**\brief 创建一个音视频数据注入数据*/
static AV_DataSource_t* aml_create_data_source(const char *fname)
{
	AV_DataSource_t *src;
#if 0	
	src = (AV_DataSource_t*)malloc(sizeof(AV_DataSource_t));
	if(!src)
	{
		AM_DEBUG(1, "not enough memory");
		return NULL;
	}
	
	memset(src, 0, sizeof(AV_DataSource_t));
	
	src->fd = open(fname, O_RDWR);
	if(src->fd==-1)
	{
		AM_DEBUG(1, "cannot open file \"%s\"", fname);
		free(src);
		return NULL;
	}
	
	pthread_mutex_init(&src->lock, NULL);
	pthread_cond_init(&src->cond, NULL);
#endif 	
	return src;
}

/**\brief 运行数据注入线程*/
static AM_ErrorCode_t aml_start_data_source(AV_DataSource_t *src, const uint8_t *data, int len, int times)
{
	int ret;
#if 0	
	if(!src->running)
	{
		src->running = AM_TRUE;
		src->data    = data;
		src->len     = len;
		src->times   = times;
		src->pos     = 0;
		
		ret = pthread_create(&src->thread, NULL, aml_data_source_thread, src);
		if(ret)
		{
			AM_DEBUG(1, "create the thread failed");
			src->running = AM_FALSE;
			return AM_AV_ERR_SYS;
		}
	}
	else
	{
		pthread_mutex_lock(&src->lock);
		
		src->data  = data;
		src->len   = len;
		src->times = times;
		src->pos   = 0;
		
		pthread_mutex_unlock(&src->lock);
		pthread_cond_signal(&src->cond);
	}
#endif	
	return AM_SUCCESS;
}

/**\brief 释放数据注入数据*/
static void aml_destroy_data_source(AV_DataSource_t *src)
{
#if 0
	if(src->running)
	{
		src->running = AM_FALSE;
		pthread_cond_signal(&src->cond);
		pthread_join(src->thread, NULL);
	}
	
	close(src->fd);
	pthread_mutex_destroy(&src->lock);
	pthread_cond_destroy(&src->cond);
	free(src);
#endif
}

/**\brief 播放器消息回调*/
static int aml_fp_msg_cb(void *obj,void *msgt)
{
	AV_FilePlayerData_t *fp = (AV_FilePlayerData_t*)obj;
#if 0	
	if(msgt !=NULL)
	{
		AM_DEBUG(1, "MPlayer event %d", msgt->head.type);
		switch(msgt->head.type)
		{
			case MP_RESP_STATE_CHANGED:
				AM_DEBUG(1, "MPlayer state changed to %d", ((MP_StateChangedRespBody*)msgt->auxDat)->state);
              			pthread_mutex_lock(&fp->lock);
              			
				fp->state = ((MP_StateChangedRespBody*)msgt->auxDat)->state;
				AM_EVT_Signal(fp->av->dev_no, AM_AV_EVT_PLAYER_STATE_CHANGED, (void*)((MP_StateChangedRespBody*)msgt->auxDat)->state);
				
				pthread_mutex_unlock(&fp->lock);
				pthread_cond_broadcast(&fp->cond);
			break;
			case MP_RESP_DURATION:
				pthread_mutex_lock(&fp->lock);
				if(fp->resp_type==AV_MP_RESP_DURATION)
				{
					fp->resp_data.duration = *(int*)msgt->auxDat;
					fp->resp_type = AV_MP_RESP_OK;
					pthread_cond_broadcast(&fp->cond);
				}
				pthread_mutex_unlock(&fp->lock);
			break;
			case MP_RESP_TIME_CHANGED:	
				pthread_mutex_lock(&fp->lock);
				if(fp->resp_type==AV_MP_RESP_POSITION)
				{
					fp->resp_data.position = *(int*)msgt->auxDat;
					fp->resp_type = AV_MP_RESP_OK;
					pthread_cond_broadcast(&fp->cond);
				}
				AM_EVT_Signal(fp->av->dev_no, AM_AV_EVT_PLAYER_TIME_CHANGED, (void*)*(int*)msgt->auxDat);
				pthread_mutex_unlock(&fp->lock);
			break;
			case MP_RESP_MEDIAINFO:
				pthread_mutex_lock(&fp->lock);
				if(fp->resp_type==AV_MP_RESP_MEDIA)
				{
					MP_AVMediaFileInfo *info = (MP_AVMediaFileInfo*)msgt->auxDat;
					fp->resp_data.media = *info;
					fp->resp_type = AV_MP_RESP_OK;
					pthread_cond_broadcast(&fp->cond);
				}
				pthread_mutex_unlock(&fp->lock);
			break;
			default:
			break;
		}
		
		MP_free_response_msg(msgt);
	}
#endif	
	return 0;
}

/**\brief 释放文件播放数据*/
static void aml_destroy_fp(AV_FilePlayerData_t *fp)
{
	int rc;
#if 0	
	/*等待播放器停止*/
	if(fp->media_id!=-1)
	{
		pthread_mutex_lock(&fp->lock);
		
		fp->resp_type= AV_MP_RESP_STOP;
		rc = MP_stop(fp->media_id);
		
		if(rc==0)
		{
			while((fp->state!=MP_STATE_STOPED) && (fp->state!=MP_STATE_NORMALERROR) &&
					(fp->state!=MP_STATE_FATALERROR) && (fp->state!=MP_STATE_FINISHED))
				pthread_cond_wait(&fp->cond, &fp->lock);
		}
		
		pthread_mutex_unlock(&fp->lock);
		
		MP_CloseMediaID(fp->media_id);
	}
	
	pthread_mutex_destroy(&fp->lock);
	pthread_cond_destroy(&fp->cond);
	
	MP_ReleaseMPClient();
	
	free(fp);
#endif
}

/**\brief 创建文件播放器数据*/
static AV_FilePlayerData_t* aml_create_fp(AM_AV_Device_t *dev)
{
	AV_FilePlayerData_t *fp;
#if 0	
	fp = (AV_FilePlayerData_t*)malloc(sizeof(AV_FilePlayerData_t));
	if(!fp)
	{
		AM_DEBUG(1, "not enough memory");
		return NULL;
	}
	
	if(MP_CreateMPClient()==-1)
	{
		AM_DEBUG(1, "MP_CreateMPClient failed");
		free(fp);
		return NULL;
	}
	
	memset(fp, 0, sizeof(AV_FilePlayerData_t));
	pthread_mutex_init(&fp->lock, NULL);
	pthread_cond_init(&fp->cond, NULL);
	
	fp->av       = dev;
	fp->media_id = -1;
	
	MP_RegPlayerRespMsgListener(fp, aml_fp_msg_cb);
#endif	
	return fp;
}

/**\brief 初始化JPEG解码器*/
static AM_ErrorCode_t aml_init_jpeg(AV_JPEGData_t *jpeg)
{
#if 0
	if(jpeg->dec_fd!=-1)
	{
		close(jpeg->dec_fd);
		jpeg->dec_fd = -1;
	}
	
	if(jpeg->vbuf_fd!=-1)
	{
		close(jpeg->vbuf_fd);
		jpeg->vbuf_fd = -1;
	}
	
	jpeg->vbuf_fd = open(STREAM_VBUF_FILE, O_RDWR|O_NONBLOCK);
	if(jpeg->vbuf_fd==-1)
	{
		AM_DEBUG(1, "cannot open amstream_vbuf");
		goto error;
	}
	
	if(ioctl(jpeg->vbuf_fd, AMSTREAM_IOC_VFORMAT, VFORMAT_JPEG)==-1)
	{
		AM_DEBUG(1, "set jpeg video format failed (\"%s\")", strerror(errno));
		goto error;
	}
	
	if(ioctl(jpeg->vbuf_fd, AMSTREAM_IOC_PORT_INIT)==-1)
	{
		AM_DEBUG(1, "amstream init failed (\"%s\")", strerror(errno));
		goto error;
	}
	
	return AM_SUCCESS;
error:
	if(jpeg->dec_fd!=-1)
	{
		close(jpeg->dec_fd);
		jpeg->dec_fd = -1;
	}
	if(jpeg->vbuf_fd!=-1)
	{
		close(jpeg->vbuf_fd);
		jpeg->vbuf_fd = -1;
	}
#endif
	return AM_AV_ERR_SYS;
}

/**\brief 创建JPEG解码相关数据*/
static AV_JPEGData_t* aml_create_jpeg_data(void)
{
	AV_JPEGData_t *jpeg;
#if 0	
	jpeg = malloc(sizeof(AV_JPEGData_t));
	if(!jpeg)
	{
		AM_DEBUG(1, "not enough memory");
		return NULL;
	}
	
	jpeg->vbuf_fd = -1;
	jpeg->dec_fd  = -1;
	
	if(aml_init_jpeg(jpeg)!=AM_SUCCESS)
	{
		free(jpeg);
		return NULL;
	}
#endif	
	return jpeg;
}

/**\brief 释放JPEG解码相关数据*/
static void aml_destroy_jpeg_data(AV_JPEGData_t *jpeg)
{
#if 0
	if(jpeg->dec_fd!=-1)
		close(jpeg->dec_fd);
	if(jpeg->vbuf_fd!=-1)
	{
		close(jpeg->vbuf_fd);
	}
	free(jpeg);
#endif
}

extern unsigned long CMEM_getPhys(unsigned long virts);

/**\brief 解码JPEG数据*/
static AM_ErrorCode_t aml_decode_jpeg(AV_JPEGData_t *jpeg, const uint8_t *data, int len, int mode, void *para)
{
	AV_JPEGDecodePara_t *dec_para = (AV_JPEGDecodePara_t*)para;
	AV_JPEGDecState_t s;
	AM_Bool_t decLoop = AM_TRUE;
	int decState = 0;
	int try_count = 0;
	int decode_wait = 0;
	const uint8_t *src = data;
	int left = len;
	AM_ErrorCode_t ret = AM_SUCCESS;
	AM_OSD_Surface_t *surf = NULL;
	jpegdec_info_t info;
	char tmp_buf[64];
#if 0	
	s = AV_JPEG_DEC_STAT_INFOCONFIG;
	while(decLoop)
	{
		if(jpeg->dec_fd==-1)
		{
			jpeg->dec_fd = open(JPEG_DEC_FILE, O_RDWR);
			if (jpeg->dec_fd!=-1)
			{
				s = AV_JPEG_DEC_STAT_INFOCONFIG;
			}
			else
			{
				try_count++;
				if(try_count>40)
				{
					AM_DEBUG(1, "jpegdec init timeout");
					try_count=0;
					ret = aml_init_jpeg(jpeg);
					if(ret!=AM_SUCCESS)
						break;
				}
				usleep(10000);
				continue;
			}
		}
		else
		{
			decState = ioctl(jpeg->dec_fd, JPEGDEC_IOC_STAT);
			
			if (decState & JPEGDEC_STAT_ERROR)
			{
				AM_DEBUG(1, "jpegdec JPEGDEC_STAT_ERROR");
				ret = AM_AV_ERR_DECODE;
				break;
			}

			if (decState & JPEGDEC_STAT_UNSUPPORT)
			{
				AM_DEBUG(1, "jpegdec JPEGDEC_STAT_UNSUPPORT");
				ret = AM_AV_ERR_DECODE;
				break;
			}

			if (decState & JPEGDEC_STAT_DONE)
				break;
			
			if (decState & JPEGDEC_STAT_WAIT_DATA) 
			{
				if(left > 0) 
				{
					int send = AM_MIN(left, JPEG_WRTIE_UNIT);
					int rc;
					rc = write(jpeg->vbuf_fd, src, send);
					if(rc==-1) 
					{
						AM_DEBUG(1, "write data to the jpeg decoder failed");
						ret = AM_AV_ERR_DECODE;
						break;
					}
					left -= rc;
					src  += rc;
				}
				else if(decode_wait==0)
				{
					int i, times = JPEG_WRTIE_UNIT/sizeof(tmp_buf);
					
					memset(tmp_buf, 0, sizeof(tmp_buf));
					
					for(i=0; i<times; i++)
						write(jpeg->vbuf_fd, tmp_buf, sizeof(tmp_buf));
					decode_wait++;
				}
				else
				{
					if(decode_wait>300)
					{
						AM_DEBUG(1, "jpegdec wait data error");
						ret = AM_AV_ERR_DECODE;
						break;
					}
					decode_wait++;
					usleep(10);
				}
			}
	
			switch (s)
			{
				case AV_JPEG_DEC_STAT_INFOCONFIG:
					if (decState & JPEGDEC_STAT_WAIT_INFOCONFIG) 
					{
						if(ioctl(jpeg->dec_fd, JPEGDEC_IOC_INFOCONFIG, 0)==-1)
						{
							AM_DEBUG(1, "jpegdec JPEGDEC_IOC_INFOCONFIG error");
							ret = AM_AV_ERR_DECODE;
							decLoop = AM_FALSE;
						}
						s = AV_JPEG_DEC_STAT_INFO;
					}
					break;
				case AV_JPEG_DEC_STAT_INFO:
					if (decState & JPEGDEC_STAT_INFO_READY) 
					{
						if(ioctl(jpeg->dec_fd, JPEGDEC_IOC_INFO, &info)==-1)
						{
							AM_DEBUG(1, "jpegdec JPEGDEC_IOC_INFO error");
							ret = AM_AV_ERR_DECODE;
							decLoop = AM_FALSE;
						}
						if(mode&AV_GET_JPEG_INFO)
						{
							AM_AV_JPEGInfo_t *jinfo = (AM_AV_JPEGInfo_t*)para;
							jinfo->width    = info.width;
							jinfo->height   = info.height;
							jinfo->comp_num = info.comp_num;
							decLoop = AM_FALSE;
						}
						AM_DEBUG(2, "jpegdec width:%d height:%d", info.width, info.height);
						s = AV_JPEG_DEC_STAT_DECCONFIG;
					}
					break;
				case AV_JPEG_DEC_STAT_DECCONFIG:
					if (decState & JPEGDEC_STAT_WAIT_DECCONFIG) 
					{
						jpegdec_config_t config;
						int dst_w, dst_h;
						
						switch(dec_para->angle)
						{
							case AM_AV_JPEG_CLKWISE_0:
							default:
								dst_w = info.width;
								dst_h = info.height;
							break;
							case AM_AV_JPEG_CLKWISE_90:
								dst_w = info.height;
								dst_h = info.width;
							break;
							case AM_AV_JPEG_CLKWISE_180:
								dst_w = info.width;
								dst_h = info.height;
							break;
							case AM_AV_JPEG_CLKWISE_270:
								dst_w = info.height;
								dst_h = info.width;
							break;
						}
						
						ret = AM_OSD_CreateSurface(AM_OSD_FMT_YUV_420, dst_w, dst_h, AM_OSD_SURFACE_FL_HW, &surf);
						if(ret!=AM_SUCCESS)
						{
							AM_DEBUG(1, "cannot create the YUV420 surface");
							decLoop = AM_FALSE;
						}
						else
						{
							config.addr_y = CMEM_getPhys((unsigned long)surf->buffer);
							config.addr_u = config.addr_y+CANVAS_ALIGN(surf->width)*surf->height;
							config.addr_v = config.addr_u+CANVAS_ALIGN(surf->width/2)*(surf->height/2);
							config.opt    = 0;
							config.dec_x  = 0;
							config.dec_y  = 0;
							config.dec_w  = surf->width;
							config.dec_h  = surf->height;
							config.angle  = dec_para->angle;
							config.canvas_width = CANVAS_ALIGN(surf->width);
						
							if(ioctl(jpeg->dec_fd, JPEGDEC_IOC_DECCONFIG, &config)==-1)
							{
								AM_DEBUG(1, "jpegdec JPEGDEC_IOC_DECCONFIG error");
								ret = AM_AV_ERR_DECODE;
								decLoop = AM_FALSE;
							}
							s = AV_JPEG_DEC_STAT_RUN;
						}
					}
				break;

				default:
					break;
			}
		}
	}
	
	if(surf)
	{
		if(ret==AM_SUCCESS)
		{
			dec_para->surface = surf;
		}
		else
		{
			AM_OSD_DestroySurface(surf);
		}
	}
#endif	
	return ret;
}

static AM_ErrorCode_t aml_open(AM_AV_Device_t *dev, const AM_AV_OpenPara_t *para)
{
	char buf[32];
	int v;
#if 0	
	if(AM_FileRead(VID_AXIS_FILE, buf, sizeof(buf))==AM_SUCCESS)
	{
		int left, top, right, bottom;
		
		if(sscanf(buf, "%d %d %d %d", &left, &top, &right, &bottom)==4)
		{
			dev->video_x = left;
			dev->video_y = top;
			dev->video_w = right-left;
			dev->video_h = bottom-top;
		}
	}
	if(AM_FileRead(VID_CONTRAST_FILE, buf, sizeof(buf))==AM_SUCCESS)
	{
		if(sscanf(buf, "%d", &v)==1)
		{
			dev->video_contrast = v;
		}
	}
	if(AM_FileRead(VID_SATURATION_FILE, buf, sizeof(buf))==AM_SUCCESS)
	{
		if(sscanf(buf, "%d", &v)==1)
		{
			dev->video_saturation = v;
		}
	}
	if(AM_FileRead(VID_BRIGHTNESS_FILE, buf, sizeof(buf))==AM_SUCCESS)
	{
		if(sscanf(buf, "%d", &v)==1)
		{
			dev->video_brightness = v;
		}
	}
	if(AM_FileRead(VID_DISABLE_FILE, buf, sizeof(buf))==AM_SUCCESS)
	{
		if(sscanf(buf, "%d", &v)==1)
		{
			dev->video_enable = v?AM_FALSE:AM_TRUE;
		}
	}
	if(AM_FileRead(VID_BLACKOUT_FILE, buf, sizeof(buf))==AM_SUCCESS)
	{
		if(sscanf(buf, "%d", &v)==1)
		{
			dev->video_blackout = v?AM_TRUE:AM_FALSE;
		}
	}
	if(AM_FileRead(VID_SCREEN_MODE_FILE, buf, sizeof(buf))==AM_SUCCESS)
	{
		if(sscanf(buf, "%d", &v)==1)
		{
			dev->video_mode = v;
		}
	}
	if(AM_FileRead(DISP_MODE_FILE, buf, sizeof(buf))==AM_SUCCESS)
	{
		if(!strncmp(buf, "576cvbs", 7) || !strncmp(buf, "576i", 4) || !strncmp(buf, "576i", 4))
		{
			dev->vout_w = 720;
			dev->vout_h = 576;
		}
		else if (!strncmp(buf, "480cvbs", 7) || !strncmp(buf, "480i", 4) || !strncmp(buf, "480i", 4))
		{
			dev->vout_w = 720;
			dev->vout_h = 480;
		}
		else if (!strncmp(buf, "720p", 4))
		{
			dev->vout_w = 1280;
			dev->vout_h = 720;
		}
		else if (!strncmp(buf, "1080i", 5) || !strncmp(buf, "1080p", 5))
		{
			dev->vout_w = 1920;
			dev->vout_h = 1080;
		}
	}
	
	adec_cmd("stop");
#endif
	//adec_init(NULL);
	return AM_SUCCESS;
}

static AM_ErrorCode_t aml_close(AM_AV_Device_t *dev)
{
	return AM_SUCCESS;
}

static AM_ErrorCode_t aml_open_mode(AM_AV_Device_t *dev, AV_PlayMode_t mode)
{
	AV_DataSource_t *src;
	AV_FilePlayerData_t *data;
	AV_JPEGData_t *jpeg;
	int fd;
	
	switch(mode)
	{
#if 0
		case AV_PLAY_VIDEO_ES:
			src = aml_create_data_source(STREAM_VBUF_FILE);
			if(!src)
			{
				AM_DEBUG(1, "cannot create data source \"/dev/amstream_vbuf\"");
				return AM_AV_ERR_CANNOT_OPEN_DEV;
			}
			ioctl(src->fd, AMSTREAM_IOC_TRICKMODE, TRICKMODE_I);
			dev->vid_player.drv_data = src;
		break;
		case AV_PLAY_AUDIO_ES:
			src = aml_create_data_source(STREAM_ABUF_FILE);
			if(!src)
			{
				AM_DEBUG(1, "cannot create data source \"/dev/amstream_abuf\"");
				return AM_AV_ERR_CANNOT_OPEN_DEV;
			}
			dev->aud_player.drv_data = src;
		break;
#endif
		case AV_PLAY_TS:
			fd = open(STREAM_TS_FILE, O_RDWR);
			if(fd==-1)
			{
				AM_DEBUG(1, "cannot open \"/dev/amstream_mpts\"");
				return AM_AV_ERR_CANNOT_OPEN_DEV;
			}
			dev->ts_player.drv_data = (void*)fd;
		break;
#if 0
		case AV_PLAY_FILE:
			data = aml_create_fp(dev);
			if(!data)
			{
				AM_DEBUG(1, "not enough memory");
				return AM_AV_ERR_NO_MEM;
			}
			dev->file_player.drv_data = data;
		break;
		case AV_GET_JPEG_INFO:
		case AV_DECODE_JPEG:
			jpeg = aml_create_jpeg_data();
			if(!jpeg)
			{
				AM_DEBUG(1, "not enough memory");
				return AM_AV_ERR_NO_MEM;
			}
			dev->vid_player.drv_data = jpeg;
		break;
#endif
		default:
		break;
	}
	
	return AM_SUCCESS;
}

static AM_ErrorCode_t aml_start_mode(AM_AV_Device_t *dev, AV_PlayMode_t mode, void *para)
{
	AV_DataSource_t *src;
	int fd, val;
	AV_TSPlayPara_t *tp;
	AV_FilePlayerData_t *data;
	AV_FilePlayPara_t *pp;
	AV_JPEGData_t *jpeg;
	codec_para_t pcodec;
	switch(mode)
	{
#if 0
		case AV_PLAY_VIDEO_ES:
			src = dev->vid_player.drv_data;
			val = (int)para;
			if(ioctl(src->fd, AMSTREAM_IOC_VFORMAT, val)==-1)
			{
				AM_DEBUG(1, "set video format failed");
				return AM_AV_ERR_SYS;
			}
			aml_start_data_source(src, dev->vid_player.para.data, dev->vid_player.para.len, dev->vid_player.para.times+1);
		break;
		case AV_PLAY_AUDIO_ES:
			src = dev->aud_player.drv_data;
			val = (int)para;
			if(ioctl(src->fd, AMSTREAM_IOC_AFORMAT, val)==-1)
			{
				AM_DEBUG(1, "set audio format failed");
				return AM_AV_ERR_SYS;
			}
			aml_start_data_source(src, dev->aud_player.para.data, dev->aud_player.para.len, dev->aud_player.para.times);
			AM_AOUT_SetDriver(AOUT_DEV_NO, &adec_aout_drv, NULL);
			adec_cmd("start");
		break;
#endif
		case AV_PLAY_TS:
			fd = (int)dev->ts_player.drv_data;
			tp = (AV_TSPlayPara_t*) para;
			//adec_stop();
			//AM_AOUT_SetDriver(AOUT_DEV_NO, &adec_aout_drv, NULL);
			log_print("############### Start AV_PLAY_TS\n");
			val = tp->vfmt;
			if(ioctl(fd, AMSTREAM_IOC_VFORMAT, val)==-1)
			{
        log_print("set video format failed\n");
				return AM_AV_ERR_SYS;
			}
			val = tp->vpid;
			if(ioctl(fd, AMSTREAM_IOC_VID, val)==-1)
			{
        log_print("set video PID failed\n");
				return AM_AV_ERR_SYS;
			}
			val = tp->afmt;
			if(ioctl(fd, AMSTREAM_IOC_AFORMAT, val)==-1)
			{
				log_print("set audio format failed\n");
				return AM_AV_ERR_SYS;
			}
			val = tp->apid;
			if(ioctl(fd, AMSTREAM_IOC_AID, val)==-1)
			{
				log_print("set audio PID failed\n");
				return AM_AV_ERR_SYS;
			}
			if(ioctl(fd, AMSTREAM_IOC_PORT_INIT, 0)==-1)
			{
				log_print("amport init failed\n");
				return AM_AV_ERR_SYS;
			}
			//adec_cmd("start");
			if(tp->apid) {
        log_print("############### has audio\n");
        has_audio = 1;
			  usleep(1500000);
			  //adec_start();
        audio_start(&adec_priv, &pcodec);
      }
		break;
#if 0
		case AV_PLAY_FILE:
			data = (AV_FilePlayerData_t*)dev->file_player.drv_data;
			pp = (AV_FilePlayPara_t*)para;
			
			if(pp->start)
				data->media_id = MP_PlayMediaSource(pp->name, pp->loop, 0);
			else
				data->media_id = MP_AddMediaSource(pp->name);
			
			if(data->media_id==-1)
			{
				AM_DEBUG(1, "play file failed");
				return AM_AV_ERR_SYS;
			}
			
			AM_AOUT_SetDriver(AOUT_DEV_NO, &amplayer_aout_drv, (void*)data->media_id);
		break;
		case AV_GET_JPEG_INFO:
		case AV_DECODE_JPEG:
			jpeg = dev->vid_player.drv_data;
			return aml_decode_jpeg(jpeg, dev->vid_player.para.data, dev->vid_player.para.len, mode, para);
		break;
#endif
		default:
		break;
	}
	
	return AM_SUCCESS;
}

static AM_ErrorCode_t aml_close_mode(AM_AV_Device_t *dev, AV_PlayMode_t mode)
{
	AV_DataSource_t *src;
	AV_FilePlayerData_t *data;
	AV_JPEGData_t *jpeg;
	int fd;
	
	switch(mode)
	{
#if 0
		case AV_PLAY_VIDEO_ES:
			src = dev->vid_player.drv_data;
			ioctl(src->fd, AMSTREAM_IOC_TRICKMODE, TRICKMODE_NONE);
			aml_destroy_data_source(src);
		break;
		case AV_PLAY_AUDIO_ES:
			src = dev->aud_player.drv_data;
			adec_cmd("stop");
			aml_destroy_data_source(src);
		break;
#endif
    case AV_PLAY_TS:
			fd = (int)dev->ts_player.drv_data;
      log_print("######### aml_close_mode\n");
      if(has_audio) {
        //adec_cmd("stop");
			  //adec_stop();
        audio_stop(&adec_priv);
        adec_priv = NULL;
        has_audio = 0;
      }
			close(fd);
		break;
#if 0
		case AV_PLAY_FILE:
			data = (AV_FilePlayerData_t*)dev->file_player.drv_data;
			aml_destroy_fp(data);
			adec_cmd("stop");
		break;
		case AV_GET_JPEG_INFO:
		case AV_DECODE_JPEG:
			jpeg = dev->vid_player.drv_data;
			aml_destroy_jpeg_data(jpeg);
		break;
#endif
		default:
		break;
	}
	
	return AM_SUCCESS;
}

static AM_ErrorCode_t aml_ts_source(AM_AV_Device_t *dev, AM_AV_TSSource_t src)
{
	char *cmd;
	
	switch(src)
	{
		case AM_AV_TS_SRC_TS0:
			cmd = "ts0";
		break;
		case AM_AV_TS_SRC_TS1:
			cmd = "ts1";
		break;
		case AM_AV_TS_SRC_TS2:
			cmd = "ts2";
		break;
		case AM_AV_TS_SRC_HIU:
			cmd = "hiu";
		break;
		default:
			AM_DEBUG(1, "illegal ts source %d", src);
			return AM_AV_ERR_NOT_SUPPORTED;
		break;
	}
	
	return AM_FileEcho("/sys/class/stb/source", cmd);
}

static AM_ErrorCode_t aml_file_cmd(AM_AV_Device_t *dev, AV_PlayCmd_t cmd, void *para)
{
	AV_FilePlayerData_t *data;
	AV_FileSeekPara_t *sp;
	int rc;
	
#if 0
	data = (AV_FilePlayerData_t*)dev->file_player.drv_data;
	if(data->media_id==-1)
	{
		AM_DEBUG(1, "player do not start");
		return AM_AV_ERR_SYS;
	}
	
	switch(cmd)
	{
		case AV_PLAY_START:
			rc = MP_start(data->media_id);
		break;
		case AV_PLAY_PAUSE:
			rc = MP_pause(data->media_id);
		break;
		case AV_PLAY_RESUME:
			rc = MP_resume(data->media_id);
		break;
		case AV_PLAY_FF:
			rc = MP_fastforward(data->media_id, (int)para);
		break;
		case AV_PLAY_FB:
			rc = MP_rewind(data->media_id, (int)para);
		break;
		case AV_PLAY_SEEK:
			sp = (AV_FileSeekPara_t*)para;
			rc = MP_seek(data->media_id, sp->pos, sp->start);
		break;
		default:
			AM_DEBUG(1, "illegal media player command");
			return AM_AV_ERR_NOT_SUPPORTED;
		break;
	}
	
	if(rc==-1)
	{
		AM_DEBUG(1, "player operation failed");
		return AM_AV_ERR_SYS;
	}
#endif
	
	return AM_SUCCESS;
}

static AM_ErrorCode_t aml_file_status(AM_AV_Device_t *dev, AM_AV_PlayStatus_t *status)
{
	AV_FilePlayerData_t *data;
	int rc;
	
#if 0
	data = (AV_FilePlayerData_t*)dev->file_player.drv_data;
	if(data->media_id==-1)
	{
		AM_DEBUG(1, "player do not start");
		return AM_AV_ERR_SYS;
	}
	
	/*Get duration*/
	pthread_mutex_lock(&data->lock);
	
	data->resp_type = AV_MP_RESP_DURATION;
	rc = MP_GetDuration(data->media_id);
	if(rc==0)
	{
		while(data->resp_type==AV_MP_RESP_DURATION)
			pthread_cond_wait(&data->cond, &data->lock);
	}
	
	status->duration = data->resp_data.duration;
	
	pthread_mutex_unlock(&data->lock);
	
	if(rc!=0)
	{
		AM_DEBUG(1, "get duration failed");
		return AM_AV_ERR_SYS;
	}
	
	/*Get position*/
	pthread_mutex_lock(&data->lock);
	
	data->resp_type = AV_MP_RESP_POSITION;
	rc = MP_GetPosition(data->media_id);
	if(rc==0)
	{
		while(data->resp_type==AV_MP_RESP_POSITION)
			pthread_cond_wait(&data->cond, &data->lock);
	}
	
	status->position = data->resp_data.position;
	
	pthread_mutex_unlock(&data->lock);
	
	if(rc!=0)
	{
		AM_DEBUG(1, "get position failed");
		return AM_AV_ERR_SYS;
	}
#endif	
	return AM_SUCCESS;
}

static AM_ErrorCode_t aml_file_info(AM_AV_Device_t *dev, AM_AV_FileInfo_t *info)
{
	AV_FilePlayerData_t *data;
	int rc;
#if 0	
	data = (AV_FilePlayerData_t*)dev->file_player.drv_data;
	if(data->media_id==-1)
	{
		AM_DEBUG(1, "player do not start");
		return AM_AV_ERR_SYS;
	}
	
	pthread_mutex_lock(&data->lock);
	
	data->resp_type = AV_MP_RESP_MEDIA;
	rc = MP_GetMediaInfo(dev->file_player.name, data->media_id);
	if(rc==0)
	{
		while(data->resp_type==AV_MP_RESP_MEDIA)
			pthread_cond_wait(&data->cond, &data->lock);
		
		info->size     = data->resp_data.media.size;
		info->duration = data->resp_data.media.duration;
	}
	
	pthread_mutex_unlock(&data->lock);
	
	if(rc!=0)
	{
		AM_DEBUG(1, "get media information failed");
		return AM_AV_ERR_SYS;
	}
#endif	
	return AM_SUCCESS;
}

static AM_ErrorCode_t aml_set_video_para(AM_AV_Device_t *dev, AV_VideoParaType_t para, void *val)
{
	const char *name = NULL, *cmd;
	char buf[32];
	AM_ErrorCode_t ret = AM_SUCCESS;
	AV_VideoWindow_t *win;
#if 0	
	switch(para)
	{
		case AV_VIDEO_PARA_WINDOW:
			name = VID_AXIS_FILE;
			win = (AV_VideoWindow_t*)val;
			snprintf(buf, sizeof(buf), "%d %d %d %d", win->x, win->y, win->x+win->w, win->y+win->h);
			cmd = buf;
		break;
		case AV_VIDEO_PARA_CONTRAST:
			name = VID_CONTRAST_FILE;
			snprintf(buf, sizeof(buf), "%d", (int)val);
			cmd = buf;
		break;
		case AV_VIDEO_PARA_SATURATION:
			name = VID_SATURATION_FILE;
			snprintf(buf, sizeof(buf), "%d", (int)val);
			cmd = buf;
		break;
		case AV_VIDEO_PARA_BRIGHTNESS:
			name = VID_BRIGHTNESS_FILE;
			snprintf(buf, sizeof(buf), "%d", (int)val);
			cmd = buf;
		break;
		case AV_VIDEO_PARA_ENABLE:
			name = VID_DISABLE_FILE;
			cmd = ((int)val)?"0":"1";
		break;
		case AV_VIDEO_PARA_BLACKOUT:
			name = VID_BLACKOUT_FILE;
			cmd = ((int)val)?"1":"0";
		break;
		case AV_VIDEO_PARA_RATIO:
			ret = AM_AV_ERR_NOT_SUPPORTED;
		break;
		case AV_VIDEO_PARA_MODE:
			name = VID_SCREEN_MODE_FILE;
			cmd = ((AM_AV_VideoDisplayMode_t)val)?"1":"0";
		break;
	}
	
	if(name)
	{
		ret = AM_FileEcho(name, cmd);
	}
#endif	
	return ret;
}

