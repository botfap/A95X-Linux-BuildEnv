

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <codec.h>
#include <stdbool.h>
#include <ctype.h>

#include "ESPlayer.h"

/*******************************************************************************************************************************
//其中视音频文件的格式为：PTS（4个字节，高位在前，单位：ms）+buffer size（4个字节，高位在前）+buffer数据，
//请按此顺序依次读出，推入你们平台的ES解码器进行解码；

//视频文件有点特殊，文件的前四个字节为0xffffffff（代表无效PTS），
//接下来4个字节同样为buffer size，接下来的buffer是H264解码所需要的SPS+PPS，再接下来的格式就和上述格式一样了
add English comment
video/audio data format: PTS data (4 byts, MSB first, pts unit: ms), video data size (4 byts, MSB first, size for only payload), data
video pts may be -1, which is not available pts, do NOT send it to decoder
********************************************************************************************************************************/

#define EXTERNAL_PTS    (1)
#define SYNC_OUTSIDE    (2)

static codec_para_t v_codec_para;
static codec_para_t a_codec_para;
static codec_para_t *pcodec = NULL;
static codec_para_t *apcodec = NULL;
static codec_para_t *vpcodec = NULL;

void ES_PlayInit()
{
	
	int ret = CODEC_ERROR_NONE;

	//Init Video Codecs

	vpcodec = &v_codec_para;
	memset(vpcodec, 0, sizeof(codec_para_t ));

	codec_audio_basic_init();

	vpcodec->has_video = 1;
	vpcodec->video_type = VFORMAT_H264;
	
	vpcodec->am_sysinfo.format = VIDEO_DEC_FORMAT_H264;

	vpcodec->am_sysinfo.param = (void *)(EXTERNAL_PTS | SYNC_OUTSIDE);

	vpcodec->stream_type = STREAM_TYPE_ES_VIDEO;

	vpcodec->am_sysinfo.rate = 96000/24 ;//96000 / atoi(argv[4]);
	//vpcodec->am_sysinfo.height = atoi(argv[3]);
	//vpcodec->am_sysinfo.width = atoi(argv[2]);

	vpcodec->has_audio = 0;
	vpcodec->noblock = 0;

	apcodec = &a_codec_para;
	
	memset(apcodec, 0, sizeof(codec_para_t ));

	apcodec->audio_type = AFORMAT_AAC;
	apcodec->stream_type = STREAM_TYPE_ES_AUDIO;
//	apcodec->audio_pid = 0x1023;
	apcodec->has_audio = 1;
	apcodec->noblock = 0;
	
	//Do NOT set audio info if we do not know it
	apcodec->audio_channels = 0;
	apcodec->audio_samplerate = 0;
	apcodec->audio_info.channels = 0;
	apcodec->audio_info.sample_rate = 0;
	//apcodec->audio_channels = 2;
	//apcodec->audio_samplerate = 44100;
	//apcodec->audio_info.channels = 2;
	//apcodec->audio_info.sample_rate = 44100;

	
	ret = codec_init(vpcodec);
	if(ret != CODEC_ERROR_NONE)
	{
		printf("v codec init failed, ret=-0x%x", -ret);
		return;
	}

	ret = codec_init(apcodec);
	if(ret != CODEC_ERROR_NONE)
	{
		printf("a codec init failed, ret=-0x%x", -ret);
		return;
	}
	
	//codec_set_av_threshold(apcodec, 180);
}

void ES_PlayDeinit()
{
	if( apcodec != NULL )
	{
		codec_close(apcodec);
	//	apcodec = NULL;
	}
	
	if( vpcodec != NULL )
	{
		codec_close(vpcodec);
		vpcodec = NULL;
	}
		
}

void ES_PlayPause()
{

}

void ES_PlayResume()
{

}

void ES_PlayFreeze ()
{
	
}

void ES_PlayResetESBuffer ()
{

}

void ES_PlayGetESBufferStatus ( int *audio_rate,int *vid_rate )
{
	
	int ret;
	struct buf_status vbuf;
	
	if( vpcodec == NULL)
	{
		*vid_rate=0;
	}
	else
	{
		ret = codec_get_vbuf_state( vpcodec, &vbuf);

		*vid_rate = ( vbuf.data_len * 100 )/vbuf.size ;
	}
	
	
	if( apcodec == NULL )
	{
		*audio_rate=0;
	}
	else
	{
		ret = codec_get_abuf_state( apcodec, &vbuf);

		*audio_rate = ( vbuf.data_len * 100 )/vbuf.size ;
	}
	
	
}

void ES_PlayInjectionMediaDatas ( MEDIA_CODEC_TYPE_E data_type, void *es_buffer, unsigned int buffer_len, unsigned int PTS )
{
	
	int ret;

	if( (DAL_ES_VCODEC_TYPE_H264 == data_type) && ( vpcodec != NULL) )
	{
		int len = buffer_len;
		int wcnt = buffer_len;
		
        if( PTS != 0xffffffff )
			ret = codec_checkin_pts( vpcodec, PTS);
	    
		while( 1 )
		{
			wcnt = codec_write( vpcodec, es_buffer, buffer_len);
			if( wcnt > 0 )
			{
				len = len - wcnt;			
				if( len <= 0 )
				{
					break;
				}

				memmove( es_buffer, (unsigned char*)es_buffer+wcnt, len );
			}
			else
			{
				break;
			}
		}
	}
	else if( (DAL_ES_ACODEC_TYPE_AAC == data_type) && ( apcodec != NULL) ) 
	{	

		int len = buffer_len;
		int wcnt = buffer_len;
        ret = codec_checkin_pts( apcodec, PTS);
		while( 1 )
		{			
			wcnt = codec_write( apcodec, es_buffer, buffer_len);
			if( wcnt > 0 )
			{
				len = len - wcnt;			
				if( len <= 0 )
				{
					break;
				}

				memmove( es_buffer, (unsigned char*)es_buffer+wcnt, len );
			}
			else
			{
				break;
			}
		}
	}
}

