/**
* @file codec_ctrl.c
* @brief  Codec control lib functions
* @author Zhou Zhi <zhi.zhou@amlogic.com>
* @version 1.0.0
* @date 2011-02-24
*/
/* Copyright (C) 2007-2011, Amlogic Inc.
* All right reserved
*
*/
#include <stdio.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/poll.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <codec_error.h>
#include <codec_type.h>
#include <codec.h>
#include <audio_priv.h>
#include "codec_h_ctrl.h"
#include <adec-external-ctrl.h>

#define SUBTITLE_EVENT
#define TS_PACKET_SIZE 188
#define DEMUX_PLAYER_SOURCE 1

/* --------------------------------------------------------------------------*/
/**
* @brief  codec_set_demux_source  set ts demux source
*
* @param[in]  pcodec    Pointer of codec parameter structure
* @param[in]  source    set 1 for player     
*
* @return     0 for success, or fail type if < 0
*/
/* --------------------------------------------------------------------------*/
static int codec_set_demux_source(codec_para_t *pcodec, int source)
{
    int ret = 0;

    ret = codec_h_ioctl(pcodec->handle, AMSTREAM_IOC_SET, AMSTREAM_SET_DEMUX, (unsigned long)source);
    if (ret < 0) {
        return ret;
    }
    return ret;
}

/* --------------------------------------------------------------------------*/
/**
* @brief  codec_change_buf_size  Change buffer size of codec device
*
* @param[in]  pcodec  Pointer of codec parameter structure
*
* @return     Success of fail error type
*/
/* --------------------------------------------------------------------------*/
static int codec_change_buf_size(codec_para_t *pcodec)
{
    int r;
    if (pcodec->abuf_size > 0) {
        r = codec_h_ioctl(pcodec->handle, AMSTREAM_IOC_SET, AMSTREAM_SET_AB_SIZE, pcodec->abuf_size);
        if (r < 0) {
            return system_error_to_codec_error(r);
        }
    }
    if (pcodec->vbuf_size > 0) {
        r = codec_h_ioctl(pcodec->handle, AMSTREAM_IOC_SET, AMSTREAM_SET_VB_SIZE, pcodec->vbuf_size);
        if (r < 0) {
            return system_error_to_codec_error(r);
        }
    }
    return CODEC_ERROR_NONE;
}
/* --------------------------------------------------------------------------*/
/**
* @brief  set_video_format  Set video format to codec device
*
* @param[in]  pcodec  Pointer of codec parameter structure
*
* @return     0 for success, or error type
*/
/* --------------------------------------------------------------------------*/
static  int set_video_format(codec_para_t *pcodec)
{
    int format = pcodec->video_type;
    int r;

    if (format < 0 || format > VFORMAT_MAX) {
        return -CODEC_ERROR_VIDEO_TYPE_UNKNOW;
    }

    r = codec_h_ioctl(pcodec->handle, AMSTREAM_IOC_SET, AMSTREAM_SET_VFORMAT, format);
    if (pcodec->video_pid >= 0) {
        r = codec_h_ioctl(pcodec->handle, AMSTREAM_IOC_SET, AMSTREAM_SET_VID, pcodec->video_pid);
        if (r < 0) {
            return system_error_to_codec_error(r);
        }
    }
    if (r < 0) {
        return system_error_to_codec_error(r);
    }
    return 0;
}

/* --------------------------------------------------------------------------*/
/**
* @brief  set_video_codec_info  Set video information(width, height...) to codec device
*
* @param[in]  pcodec  Pointer of codec parameter structure
*
* @return     0 for success, or error type
*/
/* --------------------------------------------------------------------------*/
static  int set_video_codec_info(codec_para_t *pcodec)
{
    dec_sysinfo_t am_sysinfo = pcodec->am_sysinfo;
    int r;

    r = codec_h_control(pcodec->handle, AMSTREAM_IOC_SYSINFO, (unsigned long)&am_sysinfo);
    if (r < 0) {
        return system_error_to_codec_error(r);
    }
    return 0;
}
/* --------------------------------------------------------------------------*/
/**
* @brief  set_audio_format  Set audio format to codec device
*
* @param[in]  pcodec  Pointer of codec parameter structure
*
* @return     0 for success, or error type
*/
/* --------------------------------------------------------------------------*/
static  int set_audio_format(codec_para_t *pcodec)
{
    int format = pcodec->audio_type;
    int r;
    int codec_r;

    if (format < 0 || format > AFORMAT_MAX) {
        return -CODEC_ERROR_AUDIO_TYPE_UNKNOW;
    }

    r = codec_h_ioctl(pcodec->handle, AMSTREAM_IOC_SET, AMSTREAM_SET_AFORMAT, format);
    if (r < 0) {
        codec_r = system_error_to_codec_error(r);
        print_error_msg(codec_r, errno, __FUNCTION__, __LINE__);
        return codec_r;
    }
    if (pcodec->audio_pid >= 0) {
        r = codec_h_ioctl(pcodec->handle, AMSTREAM_IOC_SET, AMSTREAM_SET_AID, pcodec->audio_pid);
        if (r < 0) {
            codec_r = system_error_to_codec_error(r);
            print_error_msg(codec_r, errno, __FUNCTION__, __LINE__);
            return codec_r;
        }
    }
    if (pcodec->audio_samplerate > 0) {
        r = codec_h_ioctl(pcodec->handle, AMSTREAM_IOC_SET, AMSTREAM_SET_SAMPLERATE, pcodec->audio_samplerate);
        if (r < 0) {
            codec_r = system_error_to_codec_error(r);
            print_error_msg(codec_r, errno, __FUNCTION__, __LINE__);
            return codec_r;
        }
    }
    if (pcodec->audio_channels > 0) {
        r = codec_h_ioctl(pcodec->handle, AMSTREAM_IOC_SET, AMSTREAM_SET_ACHANNEL, pcodec->audio_channels);
        if (r < 0) {
            codec_r = system_error_to_codec_error(r);
            print_error_msg(codec_r, errno, __FUNCTION__, __LINE__);
            return codec_r;
        }
    }
    return 0;
}

/* --------------------------------------------------------------------------*/
/**
* @brief  set_audio_info  Set audio information to codec device
*
* @param[in]  pcodec  Pointer of codec parameter structure
*
* @return     0 for success, or error type
*/
/* --------------------------------------------------------------------------*/
static int set_audio_info(codec_para_t *pcodec)
{
    int r;
    int codec_r;
    audio_info_t *audio_info = &pcodec->audio_info;
    CODEC_PRINT("set_audio_info\n");
    r = codec_h_ioctl(pcodec->handle, AMSTREAM_IOC_SET_PTR, AMSTREAM_SET_PTR_AUDIO_INFO, (unsigned long)audio_info);
    if (r < 0) {
        codec_r = system_error_to_codec_error(r);
        print_error_msg(codec_r, errno, __FUNCTION__, __LINE__);
        return codec_r;
    }
    return 0;
}

/* --------------------------------------------------------------------------*/
/**
* @brief  set_sub_format  Set subtitle format to codec device
*
* @param[in]  pcodec  Pointer of codec parameter structure
*
* @return     0 for success, or error type
*/
/* --------------------------------------------------------------------------*/
static int set_sub_format(codec_para_t *pcodec)
{
    int r;

    if (pcodec->sub_pid >= 0) {
        r = codec_h_ioctl(pcodec->handle, AMSTREAM_IOC_SET, AMSTREAM_SET_SID, pcodec->sub_pid);
        if (r < 0) {
            return system_error_to_codec_error(r);
        }

        r = codec_h_ioctl(pcodec->handle, AMSTREAM_IOC_SET, AMSTREAM_SET_SUB_TYPE, pcodec->sub_type);
        if (r < 0) {
            return system_error_to_codec_error(r);
        }
    }

    return 0;
}

/* --------------------------------------------------------------------------*/
/**
* @brief  set_ts_skip_byte  Set the number of ts skip bytes, especially for m2ts file
*
* @param[in]  pcodec  Pointer of codec parameter structure
*
* @return     0 for success, or error type
*/
/* --------------------------------------------------------------------------*/
static int set_ts_skip_byte(codec_para_t *pcodec)
{
    int r, skip_byte;

    skip_byte = pcodec->packet_size - TS_PACKET_SIZE;

    if (skip_byte < 0) {
        skip_byte = 0;
    }

    r = codec_h_ioctl(pcodec->handle, AMSTREAM_IOC_SET, AMSTREAM_SET_TS_SKIPBYTE, skip_byte);
    if (r < 0) {
        return system_error_to_codec_error(r);
    }

    return 0;
}
/* --------------------------------------------------------------------------*/
/**
* @brief  codec_check_new_cmd  Check new cmd for ioctl
*
*/
/* --------------------------------------------------------------------------*/
static inline void codec_check_new_cmd(CODEC_HANDLE handle)
{
    if (!codec_h_is_support_new_cmd()) {
        int r;
        int version = 0;
        r = codec_h_control(handle, AMSTREAM_IOC_GET_VERSION, &version);
        if ((r == 0) && (version >= 0x20000)) {
            CODEC_PRINT("codec_init amstream version : %d.%d\n", (version & 0xffff0000) >> 16, version & 0xffff);
            codec_h_set_support_new_cmd(1);
        } else {
            CODEC_PRINT("codec_init amstream use old cmd\n");
            codec_h_set_support_new_cmd(0);
        }

    }
}

/* --------------------------------------------------------------------------*/
/**
* @brief  codec_video_es_init  Initialize the codec device for es video
*
* @param[in]  pcodec  Pointer of codec parameter structure
*
* @return     Success or fail error type
*/
/* --------------------------------------------------------------------------*/
static inline int codec_video_es_init(codec_para_t *pcodec)
{
    CODEC_HANDLE handle;
    int r;
    int codec_r;
    int flags = O_WRONLY;
    if (!pcodec->has_video) {
        return CODEC_ERROR_NONE;
    }

    flags |= pcodec->noblock ? O_NONBLOCK : 0;
    if(pcodec->video_type == VFORMAT_HEVC || pcodec->video_type == VFORMAT_VP9) {
		printf("OPEN es hevc\n");
      handle = codec_h_open(CODEC_VIDEO_ES_HEVC_DEVICE, flags);
    }
    else {
		CODEC_PRINT("OPEN es DEVICE\n");
      handle = codec_h_open(CODEC_VIDEO_ES_DEVICE, flags);
    }
    
    if (handle < 0) {
        codec_r = system_error_to_codec_error(handle);
        print_error_msg(codec_r, errno, __FUNCTION__, __LINE__);
        return CODEC_OPEN_HANDLE_FAILED;
    }
    pcodec->handle = handle;

    codec_check_new_cmd(handle);

    r = set_video_format(pcodec);
    if (r < 0) {
        codec_h_close(handle);
        codec_r = system_error_to_codec_error(r);
        print_error_msg(codec_r, errno, __FUNCTION__, __LINE__);
        return codec_r;
    }
    r = set_video_codec_info(pcodec);
    if (r < 0) {
        codec_h_close(handle);
        codec_r = system_error_to_codec_error(r);
        print_error_msg(codec_r, errno, __FUNCTION__, __LINE__);
        return codec_r;
    }
    return CODEC_ERROR_NONE;
}


/* --------------------------------------------------------------------------*/
/**
* @brief  codec_audio_es_init  Initialize the codec device for es audio
*
* @param[in]  pcodec  Pointer of codec parameter structure
*
* @return     Success or fail error type
*/
/* --------------------------------------------------------------------------*/
static inline int codec_audio_es_init(codec_para_t *pcodec)
{
    CODEC_HANDLE handle;
    int r;
    int flags = O_WRONLY;
    int codec_r;
    if (!pcodec->has_audio) {
        return CODEC_ERROR_NONE;
    }

    flags |= pcodec->noblock ? O_NONBLOCK : 0;
    handle = codec_h_open(CODEC_AUDIO_ES_DEVICE, flags);
    if (handle < 0) {
        codec_r = system_error_to_codec_error(handle);
        print_error_msg(codec_r, errno, __FUNCTION__, __LINE__);
        return CODEC_OPEN_HANDLE_FAILED;
    }
    pcodec->handle = handle;

    codec_check_new_cmd(handle);

    r = set_audio_format(pcodec);
    if (r < 0) {
        codec_h_close(handle);
        codec_r = system_error_to_codec_error(r);
        print_error_msg(codec_r, errno, __FUNCTION__, __LINE__);
        return codec_r;
    }

    /*if ((pcodec->audio_type == AFORMAT_ADPCM) || (pcodec->audio_type == AFORMAT_WMAPRO) || (pcodec->audio_type == AFORMAT_WMA) || (pcodec->audio_type == AFORMAT_PCM_S16BE)
        || (pcodec->audio_type == AFORMAT_PCM_S16LE) || (pcodec->audio_type == AFORMAT_PCM_U8)||(pcodec->audio_type == AFORMAT_AMR)) {*/
      if(IS_AUIDO_NEED_EXT_INFO(pcodec->audio_type)){
        r = set_audio_info(pcodec);
        if (r < 0) {
            codec_h_close(handle);
            codec_r = system_error_to_codec_error(r);
            print_error_msg(codec_r, errno, __FUNCTION__, __LINE__);
            return codec_r;
        }
    }

    return CODEC_ERROR_NONE;
}

/* --------------------------------------------------------------------------*/
/**
* @brief  codec_sub_es_init  Initialize the codec device for es subtitle
*
* @param[in]  pcodec  Pointer of codec parameter structure
*
* @return     Success or fail error type
*/
/* --------------------------------------------------------------------------*/
static inline int codec_sub_es_init(codec_para_t *pcodec)
{
#ifdef SUBTITLE_EVENT
    int r, codec_r;

    if (pcodec->has_sub) {
        r = codec_init_sub(pcodec);
        if (r < 0) {
            codec_r = system_error_to_codec_error(r);
            print_error_msg(codec_r, errno, __FUNCTION__, __LINE__);
            return CODEC_OPEN_HANDLE_FAILED;
        }
        pcodec->handle = pcodec->sub_handle;

        pcodec->sub_pid = 0xffff; // for es, sub id is identified for es parser
        r = set_sub_format(pcodec);
        if (r < 0) {
            codec_r = system_error_to_codec_error(r);
            print_error_msg(codec_r, errno, __FUNCTION__, __LINE__);
            return codec_r;
        }

    }

#endif

    return CODEC_ERROR_NONE;
}

/* --------------------------------------------------------------------------*/
/**
* @brief  codec_ps_init  Initialize the codec device for PS
*
* @param[in]  pcodec  Pointer of codec parameter structure
*
* @return     Success or fail error type
*/
/* --------------------------------------------------------------------------*/
static inline int codec_ps_init(codec_para_t *pcodec)
{
    CODEC_HANDLE handle;
    int r;
    int flags = O_WRONLY;
    int codec_r;
    if (!((pcodec->has_video && IS_VALID_PID(pcodec->video_pid)) ||
          (pcodec->has_audio && IS_VALID_PID(pcodec->audio_pid)))) {
        return -CODEC_ERROR_PARAMETER;
    }

    flags |= pcodec->noblock ? O_NONBLOCK : 0;
    handle = codec_h_open(CODEC_PS_DEVICE, flags);
    if (handle < 0) {
        codec_r = system_error_to_codec_error(handle);
        print_error_msg(codec_r, errno, __FUNCTION__, __LINE__);
        return CODEC_OPEN_HANDLE_FAILED;
    }
    pcodec->handle = handle;

    codec_check_new_cmd(handle);

    if (pcodec->has_video) {
        r = set_video_format(pcodec);
        if (r < 0) {
            goto error1;
        }
        if ((pcodec->video_type == VFORMAT_H264)
            || (pcodec->video_type == VFORMAT_VC1)
            || (pcodec->video_type == VFORMAT_MPEG12)) {
            r = set_video_codec_info(pcodec);
            if (r < 0) {
                /*codec_h_close(handle);
                codec_r = system_error_to_codec_error(r);
                print_error_msg(codec_r, errno, __FUNCTION__, __LINE__);
                return codec_r; */
                goto error1;
            }
        }
    }
    if (pcodec->has_audio) {
        r = set_audio_format(pcodec);
        if (r < 0) {
            goto error1;
        }

        /*if ((pcodec->audio_type == AFORMAT_ADPCM) || (pcodec->audio_type == AFORMAT_WMA) || (pcodec->audio_type == AFORMAT_WMAPRO) || (pcodec->audio_type == AFORMAT_PCM_S16BE)
            || (pcodec->audio_type == AFORMAT_PCM_S16LE) || (pcodec->audio_type == AFORMAT_PCM_U8)
            || (pcodec->audio_type == AFORMAT_PCM_BLURAY)||(pcodec->audio_type == AFORMAT_AMR)) {*/
          if(IS_AUIDO_NEED_EXT_INFO(pcodec->audio_type)){
            r = set_audio_info(pcodec);
            if (r < 0) {
                goto error1;
            }
        }
    }
#ifdef SUBTITLE_EVENT
    if (pcodec->has_sub) {
        r = set_sub_format(pcodec);
        if (r < 0) {
            goto error1;
        }

        r = codec_init_sub(pcodec);
        if (r < 0) {
            goto error1;
        }
    }
#endif

    return CODEC_ERROR_NONE;
error1:
    codec_h_close(handle);
    codec_r = system_error_to_codec_error(r);
    print_error_msg(codec_r, errno, __FUNCTION__, __LINE__);
    return codec_r;

}


/* --------------------------------------------------------------------------*/
/**
* @brief  codec_ts_init  Initialize the codec device for TS
*
* @param[in]  pcodec  Pointer of codec parameter structure
*
* @return     Success or fail error type
*/
/* --------------------------------------------------------------------------*/
static inline int codec_ts_init(codec_para_t *pcodec)
{
    CODEC_HANDLE handle;
    int r;
    int flags = O_WRONLY;
    int codec_r;
    if (!((pcodec->has_video && IS_VALID_PID(pcodec->video_pid)) ||
          (pcodec->has_audio && IS_VALID_PID(pcodec->audio_pid)))) {
        return -CODEC_ERROR_PARAMETER;
    }

    flags |= pcodec->noblock ? O_NONBLOCK : 0;
    handle = codec_h_open(CODEC_TS_DEVICE, flags);
    if (handle < 0) {
        codec_r = system_error_to_codec_error(handle);
        print_error_msg(codec_r, errno, __FUNCTION__, __LINE__);
        return CODEC_OPEN_HANDLE_FAILED;
    }
    pcodec->handle = handle;

    codec_check_new_cmd(handle);

    codec_set_demux_source(pcodec, DEMUX_PLAYER_SOURCE);
    if (pcodec->has_video) {
        r = set_video_format(pcodec);
        if (r < 0) {
            goto error1;
        }
        if ((pcodec->video_type == VFORMAT_H264) || (pcodec->video_type == VFORMAT_MPEG4) || (pcodec->video_type == VFORMAT_VC1) || (pcodec->video_type == VFORMAT_AVS) || (pcodec->video_type == VFORMAT_MPEG12)) {
            r = set_video_codec_info(pcodec);
            if (r < 0) {
                codec_h_close(handle);
                codec_r = system_error_to_codec_error(r);
                print_error_msg(codec_r, errno, __FUNCTION__, __LINE__);
                return codec_r;
            }
        }
    }
    if (pcodec->has_audio) {
        r = set_audio_format(pcodec);
        if (r < 0) {
            goto error1;
        }

        /*if ((pcodec->audio_type == AFORMAT_ADPCM) || (pcodec->audio_type == AFORMAT_WMA) || (pcodec->audio_type == AFORMAT_WMAPRO) || (pcodec->audio_type == AFORMAT_PCM_S16BE)
            || (pcodec->audio_type == AFORMAT_PCM_S16LE) || (pcodec->audio_type == AFORMAT_PCM_U8)
            || (pcodec->audio_type == AFORMAT_PCM_BLURAY)||(pcodec->audio_type == AFORMAT_AMR))*/
          if(pcodec->audio_info.valid){
            r = set_audio_info(pcodec);
            if (r < 0) {
                goto error1;
            }
        }
    }

    r = set_ts_skip_byte(pcodec);
    if (r < 0) {
        goto error1;
    }

#ifdef SUBTITLE_EVENT
    if (pcodec->has_sub) {
        r = set_sub_format(pcodec);
        if (r < 0) {
            goto error1;
        }

        r = codec_init_sub(pcodec);
        if (r < 0) {
            goto error1;
        }
    }
#endif
    return CODEC_ERROR_NONE;
error1:
    codec_h_close(handle);
    codec_r = system_error_to_codec_error(r);
    print_error_msg(codec_r, errno, __FUNCTION__, __LINE__);
    return codec_r;

}

/* --------------------------------------------------------------------------*/
/**
* @brief  codec_rm_init  Initialize the codec device for RM
*
* @param[in]  pcodec  Pointer of codec parameter structure
*
* @return     Success or fail error type
*/
/* --------------------------------------------------------------------------*/
static inline int codec_rm_init(codec_para_t *pcodec)
{
    CODEC_HANDLE handle;
    int r;
    int flags = O_WRONLY;
    int codec_r;
    if (!((pcodec->has_video && IS_VALID_PID(pcodec->video_pid)) ||
          (pcodec->has_audio && IS_VALID_PID(pcodec->audio_pid)))) {
        CODEC_PRINT("codec_rm_init failed! video=%d vpid=%d audio=%d apid=%d\n", pcodec->has_video, pcodec->video_pid, pcodec->has_audio, pcodec->audio_pid);
        return -CODEC_ERROR_PARAMETER;
    }
    flags |= pcodec->noblock ? O_NONBLOCK : 0;
    handle = codec_h_open(CODEC_RM_DEVICE, flags);
    if (handle < 0) {
        codec_r = system_error_to_codec_error(handle);
        print_error_msg(codec_r, errno, __FUNCTION__, __LINE__);
        return CODEC_OPEN_HANDLE_FAILED;
    }

    pcodec->handle = handle;

    codec_check_new_cmd(handle);

    if (pcodec->has_video) {
        r = set_video_format(pcodec);
        if (r < 0) {
            goto error1;
        }

        r = set_video_codec_info(pcodec);
        if (r < 0) {
            goto error1;
        }
    }
    if (pcodec->has_audio) {
        r = set_audio_format(pcodec);
        if (r < 0) {
            goto error1;
        }
        r = set_audio_info(pcodec);
        if (r < 0) {
            goto error1;
        }
    }
    return CODEC_ERROR_NONE;

error1:
    codec_h_close(handle);
    codec_r = system_error_to_codec_error(r);
    print_error_msg(codec_r, errno, __FUNCTION__, __LINE__);
    return codec_r;
}

/* --------------------------------------------------------------------------*/
/**
* @brief  codec_init  Initialize the codec device based on stream type
*
* @param[in]  pcodec  Pointer of codec parameter structure
*
* @return     Success or fail error type
*/
/* --------------------------------------------------------------------------*/
int codec_init(codec_para_t *pcodec)
{
    int ret;
    //if(pcodec->has_audio)
    //  audio_stop();
    pcodec->handle = -1;
    pcodec->cntl_handle = -1;
    pcodec->sub_handle = -1;
    pcodec->audio_utils_handle = -1;
    if (pcodec->audio_type == AFORMAT_MPEG1 || pcodec->audio_type == AFORMAT_MPEG2) {
        pcodec->audio_type = AFORMAT_MPEG;
    }
    switch (pcodec->stream_type) {
    case STREAM_TYPE_ES_VIDEO:
        ret = codec_video_es_init(pcodec);
        break;
    case STREAM_TYPE_ES_AUDIO:
        ret = codec_audio_es_init(pcodec);
        break;
    case STREAM_TYPE_ES_SUB:
        ret = codec_sub_es_init(pcodec);
        break;
    case STREAM_TYPE_PS:
        ret = codec_ps_init(pcodec);
        break;
    case STREAM_TYPE_TS:
        ret = codec_ts_init(pcodec);
        break;
    case STREAM_TYPE_RM:
        ret = codec_rm_init(pcodec);
        break;
    case STREAM_TYPE_UNKNOW:
    default:
        return -CODEC_ERROR_STREAM_TYPE_UNKNOW;
    }
    if (ret != 0) {
        return ret;
    }
    ret = codec_init_cntl(pcodec);
    if (ret != CODEC_ERROR_NONE) {
        return ret;
    }
    ret = codec_change_buf_size(pcodec);
    if (ret != 0) {
        return -CODEC_ERROR_SET_BUFSIZE_FAILED;
    }

    ret = codec_h_ioctl(pcodec->handle, AMSTREAM_IOC_SET, AMSTREAM_PORT_INIT, 0);
    if (ret != 0) {

        return -CODEC_ERROR_INIT_FAILED;
    }
    
    if (pcodec->has_audio) {
        arm_audio_info a_ainfo;
        memset(&a_ainfo,0,sizeof(arm_audio_info));
        a_ainfo.channels=pcodec->audio_channels;
        a_ainfo.sample_rate=pcodec->audio_samplerate;
        a_ainfo.format=pcodec->audio_type;
        a_ainfo.handle=pcodec->handle;
		a_ainfo.SessionID=pcodec->SessionID;
		a_ainfo.dspdec_not_supported = pcodec->dspdec_not_supported;
		a_ainfo.droppcm_flag = 0;
        a_ainfo.bitrate    =pcodec->audio_info.bitrate;
        a_ainfo.block_align=pcodec->audio_info.block_align;
        a_ainfo.codec_id   =pcodec->audio_info.codec_id;
        if(IS_AUIDO_NEED_EXT_INFO(pcodec->audio_type))
        {
            if (pcodec->audio_type != AFORMAT_WMA && pcodec->audio_type != AFORMAT_WMAPRO && pcodec->audio_type != AFORMAT_WMAVOI)
            {
                 a_ainfo.extradata_size=pcodec->audio_info.extradata_size;
                 if(a_ainfo.extradata_size>0&&a_ainfo.extradata_size<=AUDIO_EXTRA_DATA_SIZE)
                     memcpy((char*)a_ainfo.extradata,pcodec->audio_info.extradata,a_ainfo.extradata_size);
                 else
                    a_ainfo.extradata_size=0;
            }else{
                 Asf_audio_info_t asfinfo={0};
                 asfinfo.bitrate    =pcodec->audio_info.bitrate;
                 asfinfo.block_align=pcodec->audio_info.block_align;
                 asfinfo.channels   =pcodec->audio_info.channels;
                 asfinfo.codec_id   =pcodec->audio_info.codec_id;
                 asfinfo.sample_rate=pcodec->audio_info.sample_rate;
                 asfinfo.valid      =pcodec->audio_info.valid;
                 if(pcodec->audio_info.extradata_size<=512){
                     memcpy(asfinfo.extradata,pcodec->audio_info.extradata,pcodec->audio_info.extradata_size);  
                     asfinfo.extradata_size=pcodec->audio_info.extradata_size;
                 }
                 memcpy((char*)a_ainfo.extradata,&asfinfo,sizeof(Asf_audio_info_t));
                 a_ainfo.extradata_size=sizeof(Asf_audio_info_t);
            }
        }
        audio_start(&pcodec->adec_priv, &a_ainfo);
        if(pcodec->avsync_threshold > 0)
            audio_set_avsync_threshold(pcodec->adec_priv, pcodec->avsync_threshold);
    }
    return ret;
}

void codec_audio_basic_init(void)
{
    audio_basic_init();
}

/* --------------------------------------------------------------------------*/
/**
* @brief  codec_write  Write data to codec device
*
* @param[in]  pcodec  Pointer of codec parameter structure
* @param[in]  buffer  Buffer for data to be written
* @param[in]  len     Length of the data to be written
*
* @return     Length of the written data, or fail if < 0
*/
/* --------------------------------------------------------------------------*/
int codec_write(codec_para_t *pcodec, void *buffer, int len)
{
    return codec_h_write(pcodec->handle, buffer, len);
}

/* --------------------------------------------------------------------------*/
/**
* @brief  codec_read  Read data from codec device
*
* @param[in]   pcodec  Pointer of codec parameter structure
* @param[out]  buffer  Buffer for data read from codec device
* @param[in]   len     Length of the data to be read
*
* @return      Length of the read data, or fail if < 0
*/
/* --------------------------------------------------------------------------*/
int codec_read(codec_para_t *pcodec, void *buffer, int len)
{
    return codec_h_read(pcodec->handle, buffer, len);
}

/* --------------------------------------------------------------------------*/
/**
* @brief  codec_close  Close codec device
*
* @param[in]  pcodec  Pointer of codec parameter structure
*
* @return     0 for success, or fail type if < 0
*/
/* --------------------------------------------------------------------------*/
int codec_close(codec_para_t *pcodec)
{	
    int res = 0;

    if (pcodec->has_audio) {
        audio_stop(&pcodec->adec_priv);
		CODEC_PRINT("[%s]audio stop OK!\n", __FUNCTION__);
    }
#ifdef SUBTITLE_EVENT
    if (pcodec->has_sub && pcodec->sub_handle >= 0) {
        res |= codec_close_sub_fd(pcodec->sub_handle);
    }
#endif

    res |= codec_close_cntl(pcodec);
    res |= codec_h_close(pcodec->handle);

    return res;
}

/* --------------------------------------------------------------------------*/
/**
* @brief  codec_close_audio  Close audio decoder
*
* @param[in]  pcodec  Pointer of codec parameter structure
*/
/* --------------------------------------------------------------------------*/
void codec_close_audio(codec_para_t *pcodec)
{
    if (pcodec) {
        pcodec->has_audio = 0;		
    }
    audio_stop(&pcodec->adec_priv);
    return;
}

/* --------------------------------------------------------------------------*/
/**
* @brief  codec_resume_audio  Resume audio decoder to work (etc, after pause)
*
* @param[in]  pcodec  Pointer of codec parameter structure
* @param[in]  orig    Original audio status (has audio or not)
*/
/* --------------------------------------------------------------------------*/
void codec_resume_audio(codec_para_t *pcodec, unsigned int orig)
{
    pcodec->has_audio = orig;
    if (pcodec->has_audio) {
		arm_audio_info a_ainfo;
        memset(&a_ainfo,0,sizeof(arm_audio_info));
        a_ainfo.channels=pcodec->audio_channels;
        a_ainfo.sample_rate=pcodec->audio_samplerate;
        a_ainfo.format=pcodec->audio_type;
        a_ainfo.handle=pcodec->handle;
		a_ainfo.dspdec_not_supported = pcodec->dspdec_not_supported;
        a_ainfo.bitrate    =pcodec->audio_info.bitrate;
        a_ainfo.block_align=pcodec->audio_info.block_align;
        a_ainfo.codec_id   =pcodec->audio_info.codec_id;
		if (pcodec->switch_audio_flag) {
			a_ainfo.droppcm_flag = pcodec->switch_audio_flag;
			if(pcodec->stream_type == STREAM_TYPE_TS || pcodec->stream_type == STREAM_TYPE_PS)
				a_ainfo.droppcm_flag = 0;
			pcodec->switch_audio_flag = 0;
		}
        if(IS_AUIDO_NEED_EXT_INFO(pcodec->audio_type))
        {
            if(pcodec->audio_type!=AFORMAT_WMA && pcodec->audio_type!=AFORMAT_WMAPRO)
            {
                 a_ainfo.extradata_size=pcodec->audio_info.extradata_size;
                 if(a_ainfo.extradata_size>0&&a_ainfo.extradata_size<=AUDIO_EXTRA_DATA_SIZE)
                     memcpy((char*)a_ainfo.extradata,pcodec->audio_info.extradata,a_ainfo.extradata_size);
                 else
                     a_ainfo.extradata_size=0;
            }else{
                 Asf_audio_info_t asfinfo={0};
                 asfinfo.bitrate    =pcodec->audio_info.bitrate;
                 asfinfo.block_align=pcodec->audio_info.block_align;
                 asfinfo.channels   =pcodec->audio_info.channels;
                 asfinfo.codec_id   =pcodec->audio_info.codec_id;
                 asfinfo.sample_rate=pcodec->audio_info.sample_rate;
                 asfinfo.valid	   =pcodec->audio_info.valid;
                 if(pcodec->audio_info.extradata_size<=512){
                     memcpy(asfinfo.extradata,pcodec->audio_info.extradata,pcodec->audio_info.extradata_size);  
                     asfinfo.extradata_size=pcodec->audio_info.extradata_size;
                 }
                 memcpy((char*)a_ainfo.extradata,&asfinfo,sizeof(Asf_audio_info_t));
                 a_ainfo.extradata_size=sizeof(Asf_audio_info_t);
            }
        }
        audio_start(&pcodec->adec_priv, &a_ainfo);
    }
    return;
}

/* --------------------------------------------------------------------------*/
/**
* @brief  codec_checkin_pts  Checkin pts to codec device
*
* @param[in]  pcodec  Pointer of codec parameter structure
* @param[in]  pts     Pts to be checked in
*
* @return     0 for success, or fail type
*/
/* --------------------------------------------------------------------------*/
int codec_checkin_pts(codec_para_t *pcodec, unsigned long pts)
{
    //CODEC_PRINT("[%s:%d]pts=%x(%d)\n",__FUNCTION__,__LINE__,pts,pts/90000);
    return codec_h_ioctl(pcodec->handle, AMSTREAM_IOC_SET, AMSTREAM_SET_TSTAMP, pts);
}

/* --------------------------------------------------------------------------*/
/**
* @brief  codec_get_vbuf_state  Get the state of video buffer by codec device
*
* @param[in]   p    Pointer of codec parameter structure
* @param[out]  buf  Pointer of buffer status structure to get video buffer state
*
* @return      Success or fail type
*/
/* --------------------------------------------------------------------------*/
int codec_get_vbuf_state(codec_para_t *p, struct buf_status *buf)
{
    int r;
    if(codec_h_is_support_new_cmd()) {
        struct buf_status status;
        r = codec_h_ioctl(p->handle, AMSTREAM_IOC_GET_EX, AMSTREAM_GET_EX_VB_STATUS, (unsigned long)&status);
        memcpy(buf, &status, sizeof(*buf));
    } else {
        struct am_io_param am_io;
        r = codec_h_control(p->handle, AMSTREAM_IOC_VB_STATUS, (unsigned long)&am_io);
        memcpy(buf, &am_io.status, sizeof(*buf));
    }
    return system_error_to_codec_error(r);
}
/* --------------------------------------------------------------------------*/
/**
* @brief  codec_get_abuf_state  Get the state of audio buffer by codec device
*
* @param[in]   p    Pointer of codec parameter structure
* @param[out]  buf  Pointer of buffer status structure to get audio buffer state
*
* @return      Success or fail type
*/
/* --------------------------------------------------------------------------*/
int codec_get_abuf_state(codec_para_t *p, struct buf_status *buf)
{
    int r;
    if(codec_h_is_support_new_cmd()) {
        struct buf_status status;
        r = codec_h_ioctl(p->handle, AMSTREAM_IOC_GET_EX, AMSTREAM_GET_EX_AB_STATUS, (unsigned long)&status);
        memcpy(buf, &status, sizeof(*buf));
    } else {
        struct am_io_param am_io;
        r = codec_h_control(p->handle, AMSTREAM_IOC_AB_STATUS, (unsigned long)&am_io);
        memcpy(buf, &am_io.status, sizeof(*buf));
    }
    return system_error_to_codec_error(r);
}

/* --------------------------------------------------------------------------*/
/**
* @brief  codec_get_vdec_state  Get the state of video decoder by codec device
*
* @param[in]   p     Pointer of codec parameter structure
* @param[out]  vdec  Pointer of video decoder status structure
*
* @return      Success or fail type
*/
/* --------------------------------------------------------------------------*/
int codec_get_vdec_state(codec_para_t *p, struct vdec_status *vdec)
{
    int r;
    if(codec_h_is_support_new_cmd()) {
        struct vdec_status vstatus;
        r = codec_h_ioctl(p->handle, AMSTREAM_IOC_GET_EX, AMSTREAM_GET_EX_VDECSTAT, (unsigned long)&vstatus);
        memcpy(vdec, &vstatus, sizeof(*vdec));
    } else {
        struct am_io_param am_io;
        r = codec_h_control(p->handle, AMSTREAM_IOC_VDECSTAT, (unsigned long)&am_io);
        memcpy(vdec, &am_io.vstatus, sizeof(*vdec));
    }
    if (r < 0) {
        CODEC_PRINT("[codec_get_vdec_state]error[%d]: %s\n", r, codec_error_msg(system_error_to_codec_error(r)));
    }
    return system_error_to_codec_error(r);
}

/* --------------------------------------------------------------------------*/
/**
* @brief  codec_get_adec_state  Get the state of audio decoder by codec device
*
* @param[in]   p     Pointer of codec parameter structure
* @param[out]  adec  Pointer of audio decoder status structure
*
* @return      Success or fail type
*/
/* --------------------------------------------------------------------------*/
int codec_get_adec_state(codec_para_t *p, struct adec_status *adec)
{
    if(get_audio_decoder()!=AUDIO_ARC_DECODER)
	return get_decoder_status(p->adec_priv,adec);
    int r;
    if(codec_h_is_support_new_cmd()) {
        struct adec_status astatus;
        r = codec_h_ioctl(p->handle, AMSTREAM_IOC_GET_EX, AMSTREAM_GET_EX_ADECSTAT, (unsigned long)&astatus);
        if (r == 0) {
            memcpy(adec, &astatus, sizeof(*adec));
        }
    } else {
        struct am_io_param am_io;
        r = codec_h_control(p->handle, AMSTREAM_IOC_ADECSTAT, (unsigned long)&am_io);
        if(r == 0){
            memcpy(adec, &am_io.astatus, sizeof(*adec));
        }

    } 
    return system_error_to_codec_error(r);
}

/* --------------------------------------------------------------------------*/
/**
* @brief  video_pause  Pause video playing by codec device
*
* @param[in]  p  Pointer of codec parameter structure
*
* @return     0 for success, or fail type if < 0
*/
/* --------------------------------------------------------------------------*/
static int video_pause(codec_para_t *p)
{
    CODEC_PRINT("video_pause!\n");
    return codec_h_control(p->cntl_handle, AMSTREAM_IOC_VPAUSE, 1);
}

/* --------------------------------------------------------------------------*/
/**
* @brief  video_resume  Resume video playing by codec device
*
* @param[in]  p  Pointer of codec parameter structure
*
* @return     0 for success, or fail type if < 0
*/
/* --------------------------------------------------------------------------*/
static int video_resume(codec_para_t *p)
{
    CODEC_PRINT("video_resume!\n");
    return codec_h_control(p->cntl_handle, AMSTREAM_IOC_VPAUSE, 0);
}

/* --------------------------------------------------------------------------*/
/**
* @brief  codec_pause  Pause all playing(A/V) by codec device
*
* @param[in]  p  Pointer of codec parameter structure
*
* @return     Success or fail error type
*/
/* --------------------------------------------------------------------------*/
int codec_pause(codec_para_t *p)
{
    int ret = CODEC_ERROR_NONE;
    if (p) {
        CODEC_PRINT("[codec_pause]p->has_audio=%d\n", p->has_audio);
        if(p->has_audio) {
            audio_pause(p->adec_priv);
        } 
		if(p->has_video){
            ret = video_pause(p);
        }
    } else {
        ret = CODEC_ERROR_PARAMETER;
    }
    return ret;
}
/* --------------------------------------------------------------------------*/
/**
* @brief  codec_resume  Resume playing(A/V) by codec device
*
* @param[in]  p  Pointer of codec parameter structure
*
* @return     Success or fail error type
*/
/* --------------------------------------------------------------------------*/
int codec_resume(codec_para_t *p)
{
    int ret = CODEC_ERROR_NONE;
    if (p) {
        CODEC_PRINT("[codec_resume]p->has_audio=%d\n", p->has_audio);
        if(p->has_audio){
            audio_resume(p->adec_priv);
        } 
		if(p->has_video){
            ret = video_resume(p);
        }
    } else {
        ret = CODEC_ERROR_PARAMETER;
    }
    return ret;
}

/* --------------------------------------------------------------------------*/
/**
* @brief  codec_reset  Reset codec device
*
* @param[in]  p  Pointer of codec parameter structure
*
* @return     Success or fail error type
*/
/* --------------------------------------------------------------------------*/
int codec_reset(codec_para_t *p)
{
    int ret;
    ret = codec_close(p);
    if (ret != 0) {
        return ret;
    }
    ret = codec_init(p);
    CODEC_PRINT("[%s:%d]ret=%x\n", __FUNCTION__, __LINE__, ret);
    return system_error_to_codec_error(ret);
}

/* --------------------------------------------------------------------------*/
/**
* @brief  codec_init_sub  Initialize subtile codec device
*
* @param[in]  pcodec  Pointer of codec parameter structure
*
* @return     Success or fail error type
*/
/* --------------------------------------------------------------------------*/
int codec_init_sub(codec_para_t *pcodec)
{
    CODEC_HANDLE sub_handle;
    int flags = O_WRONLY;
    flags |= pcodec->noblock ? O_NONBLOCK : 0;
    sub_handle = codec_h_open(CODEC_SUB_DEVICE, flags);
    if (sub_handle < 0) {
        CODEC_PRINT("get %s failed\n", CODEC_SUB_DEVICE);
        return system_error_to_codec_error(sub_handle);
    }

    pcodec->sub_handle = sub_handle;
    return CODEC_ERROR_NONE;
}

/* --------------------------------------------------------------------------*/
/**
* @brief  codec_open_sub_read   Open read_subtitle device which is special for read subtile data
*
* @return   Device handler, or error type if < 0
*/
/* --------------------------------------------------------------------------*/
int codec_open_sub_read(void)
{
    CODEC_HANDLE sub_handle;

    sub_handle = codec_h_open_rd(CODEC_SUB_READ_DEVICE);
    if (sub_handle < 0) {
        CODEC_PRINT("get %s failed\n", CODEC_SUB_READ_DEVICE);
        return system_error_to_codec_error(sub_handle);
    }

    return sub_handle;
}

/* --------------------------------------------------------------------------*/
/**
* @brief  codec_close_sub  Close subtile device
*
* @param[in]  pcodec  Pointer of codec parameter structure
*
* @return     Success or fail error type
*/
/* --------------------------------------------------------------------------*/
int codec_close_sub(codec_para_t *pcodec)
{
    int res = CODEC_ERROR_NONE;

    if (pcodec) {
        if (pcodec->sub_handle) {
            res = codec_h_close(pcodec->sub_handle);
        }
    }
    return res;
}

/* --------------------------------------------------------------------------*/
/**
* @brief  codec_close_sub_fd  Close subtile device by fd
*
* @param[in]  sub_fd  subtile device fd
*
* @return     0 for success, or fail type if < 0
*/
/* --------------------------------------------------------------------------*/
int codec_close_sub_fd(CODEC_HANDLE sub_fd)
{
    int res = CODEC_ERROR_NONE;

    if (sub_fd) {
        res = codec_h_close(sub_fd);
    }
    return res;
}

/* --------------------------------------------------------------------------*/
/**
* @brief  codec_poll_sub  Polling subtile device if subtitle data is ready
*
* @param[in]  pcodec  Pointer of codec parameter structure
*
* @return     Polling result
*/
/* --------------------------------------------------------------------------*/
int codec_poll_sub(codec_para_t *pcodec)
{
    struct pollfd sub_poll_fd[1];

    if (pcodec->sub_handle == 0) {
        return 0;
    }

    sub_poll_fd[0].fd = pcodec->sub_handle;
    sub_poll_fd[0].events = POLLOUT;

    return poll(sub_poll_fd, 1, 10);
}

/* --------------------------------------------------------------------------*/
/**
* @brief  codec_poll_sub_fd  Polling subtile device if subtitle data is ready by fd
*
* @param[in]  sub_fd   Subtitle device fd
* @param[in]  timeout  Timeout for polling
*
* @return     Polling result
*/
/* --------------------------------------------------------------------------*/
int codec_poll_sub_fd(CODEC_HANDLE sub_fd, int timeout)
{
    struct pollfd sub_poll_fd[1];

    if (sub_fd <= 0) {
        return 0;
    }

    sub_poll_fd[0].fd = sub_fd;
    sub_poll_fd[0].events = POLLOUT;

    return poll(sub_poll_fd, 1, timeout);
}

/* --------------------------------------------------------------------------*/
/**
* @brief  codec_get_sub_size  Get the size of subtitle data which is ready
*
* @param[in]  pcodec  Pointer of codec parameter structure
*
* @return     Subtile ready data size, or fail error type if < 0
*/
/* --------------------------------------------------------------------------*/
int codec_get_sub_size(codec_para_t *pcodec)
{
    int sub_size, r;

    if (pcodec->sub_handle == 0) {
        CODEC_PRINT("no control handler\n");
        return 0;
    }

    r = codec_h_ioctl(pcodec->sub_handle, AMSTREAM_IOC_GET, AMSTREAM_GET_SUB_LENGTH, (unsigned long)&sub_size);
    if (r < 0) {
        return system_error_to_codec_error(r);
    } else {
        return sub_size;
    }
}

/* --------------------------------------------------------------------------*/
/**
* @brief  codec_get_sub_size_fd  Get the size of subtitle data which is ready by fd
*
* @param[in]  sub_fd  Subtitle device fd
*
* @return     Subtile ready data size, or fail error type if < 0
*/
/* --------------------------------------------------------------------------*/
int codec_get_sub_size_fd(CODEC_HANDLE sub_fd)
{
    int sub_size, r;

    if (sub_fd <= 0) {
        CODEC_PRINT("no sub handler\n");
        return 0;
    }

    r = codec_h_ioctl(sub_fd, AMSTREAM_IOC_GET, AMSTREAM_GET_SUB_LENGTH, (unsigned long)&sub_size);
    if (r < 0) {
        return system_error_to_codec_error(r);
    } else {
        return sub_size;
    }
}

/* --------------------------------------------------------------------------*/
/**
* @brief  codec_read_sub_data  Read subtitle data from codec device
*
* @param[in]   pcodec  Pointer of codec parameter structure
* @param[out]  buf     Buffer for data read from subtitle codec device
* @param[in]   length  Data length to be read from subtitle codec device
*
* @return      0 for success, or fail type if < 0
*/
/* --------------------------------------------------------------------------*/
int codec_read_sub_data(codec_para_t *pcodec, char *buf, unsigned int length)
{
    int data_size = length, r, read_done = 0;

    if (pcodec->sub_handle == 0) {
        CODEC_PRINT("no control handler\n");
        return 0;
    }

    while (data_size) {
        r = codec_h_read(pcodec->sub_handle, buf + read_done, data_size);
        if (r < 0) {
            return system_error_to_codec_error(r);
        } else {
            data_size -= r;
            read_done += r;
        }
    }

    return 0;
}

/* --------------------------------------------------------------------------*/
/**
* @brief  codec_read_sub_data_fd  Read subtitle data from codec device by fd
*
* @param[in]   sub_fd  Subtitle device fd
* @param[out]  buf     Buffer for data read from subtitle codec device
* @param[in]   length  Data length to be read from subtile codec device
*
* @return      0 for success, or fail type if < 0
*/
/* --------------------------------------------------------------------------*/
int codec_read_sub_data_fd(CODEC_HANDLE sub_fd, char *buf, unsigned int length)
{
    int data_size = length, r, read_done = 0;

    if (sub_fd <= 0) {
        CODEC_PRINT("no sub handler\n");
        return 0;
    }

    while (data_size) {
        r = codec_h_read(sub_fd, buf + read_done, data_size);
        if (r < 0) {
            return system_error_to_codec_error(r);
        } else {
            data_size -= r;
            read_done += r;
        }
    }

    return 0;
}

/* --------------------------------------------------------------------------*/
/**
* @brief  codec_write_sub_data  Write subtile data to subtitle device
*
* @param[in]  pcodec  Pointer of codec parameter structure
* @param[in]  buf     Buffer for data to be written
* @param[in]  length  Length of the dat to be written
*
* @return     Write length, or fail type if < 0
*/
/* --------------------------------------------------------------------------*/
int codec_write_sub_data(codec_para_t *pcodec, char *buf, unsigned int length)
{
    if (pcodec->sub_handle == 0) {
        CODEC_PRINT("no control handler\n");
        return 0;
    }

    return codec_h_write(pcodec->sub_handle, buf, length);
}

/* --------------------------------------------------------------------------*/
/**
* @brief  codec_init_cntl  Initialize the video control device
*
* @param[in]  pcodec  Pointer of codec parameter structure
*
* @return     Success or fail error type
*/
/* --------------------------------------------------------------------------*/
int codec_init_cntl(codec_para_t *pcodec)
{
    CODEC_HANDLE cntl;


    cntl = codec_h_open(CODEC_CNTL_DEVICE, O_RDWR);
    if (cntl < 0) {
        CODEC_PRINT("get %s failed\n", CODEC_CNTL_DEVICE);
        return system_error_to_codec_error(cntl);
    }

    pcodec->cntl_handle = cntl;
    
    return CODEC_ERROR_NONE;
}

/* --------------------------------------------------------------------------*/
/**
* @brief  codec_close_cntl  Close video control device
*
* @param[in]  pcodec  Pointer of codec parameter structure
*
* @return     Success or fail error type
*/
/* --------------------------------------------------------------------------*/
int codec_close_cntl(codec_para_t *pcodec)
{
    int res = CODEC_ERROR_NONE;

    if (pcodec) {
        if (pcodec->cntl_handle) {
            res = codec_h_control(pcodec->cntl_handle, AMSTREAM_IOC_CLEAR_VIDEO, 0);
            res = codec_h_close(pcodec->cntl_handle);
		    CODEC_PRINT("[%s]video codec close return=%d!\n", __FUNCTION__,res);
        }
    }
    return res;
}

/* --------------------------------------------------------------------------*/
/**
* @brief  codec_poll_cntl  Polling video control device
*
* @param[in]  pcodec  Pointer of codec parameter structure
*
* @return     Polling results
*/
/* --------------------------------------------------------------------------*/
int codec_poll_cntl(codec_para_t *pcodec)
{
    struct pollfd codec_poll_fd[1];

    if (pcodec->cntl_handle == 0) {
        return 0;
    }

    codec_poll_fd[0].fd = pcodec->cntl_handle;
    codec_poll_fd[0].events = POLLOUT;

    return poll(codec_poll_fd, 1, 10);
}

/* --------------------------------------------------------------------------*/
/**
* @brief  codec_get_cntl_state  Get the status of video control device, especially for trickmode
*
* @param[in]  pcodec  Pointer of codec parameter structure
*
* @return     Video control device status or fail error type
*/
/* --------------------------------------------------------------------------*/
int codec_get_cntl_state(codec_para_t *pcodec)
{
    int cntl_state, r;

    if (pcodec->cntl_handle == 0) {
        CODEC_PRINT("no control handler\n");
        return 0;
    }

    r = codec_h_control(pcodec->cntl_handle, AMSTREAM_IOC_TRICK_STAT, (unsigned long)&cntl_state);
    if (r < 0) {
        return system_error_to_codec_error(r);
    } else {
        return cntl_state;
    }
}

/* --------------------------------------------------------------------------*/
/**
* @brief  codec_set_cntl_mode  Set the mode to video control device, especially for trickmode
*
* @param[in]  pcodec  Pointer of codec parameter structure
* @param[in]  mode    Trick mode to be set
*
* @return     0 for success, or fail type if < 0
*/
/* --------------------------------------------------------------------------*/
int codec_set_cntl_mode(codec_para_t *pcodec, unsigned int mode)
{
    return codec_h_control(pcodec->cntl_handle, AMSTREAM_IOC_TRICKMODE, (unsigned long)mode);
}

/* --------------------------------------------------------------------------*/
/**
* @brief  codec_set_cntl_avthresh  Set the AV sync threshold which defines the max time difference between A/V
*
* @param[in]  pcodec    Pointer of codec parameter structure
* @param[in]  avthresh  Threshold to be set
*
* @return     0 for success, or fail type if < 0
*/
/* --------------------------------------------------------------------------*/
int codec_set_cntl_avthresh(codec_para_t *pcodec, unsigned int avthresh)
{
    return codec_h_control(pcodec->cntl_handle, AMSTREAM_IOC_AVTHRESH, (unsigned long)avthresh);
}

/* --------------------------------------------------------------------------*/
/**
* @brief  codec_set_cntl_syncthresh  Set sync threshold control which defines the starting system time (hold video or not)
*                                    when playing
*
* @param[in]  pcodec      Pointer of codec parameter structure
* @param[in]  syncthresh  Sync threshold control
*
* @return     0 for success, or fail type if < 0
*/
/* --------------------------------------------------------------------------*/
int codec_set_cntl_syncthresh(codec_para_t *pcodec, unsigned int syncthresh)
{
    return codec_h_control(pcodec->cntl_handle, AMSTREAM_IOC_SYNCTHRESH, (unsigned long)syncthresh);
}

/* --------------------------------------------------------------------------*/
/**
* @brief  codec_reset_audio  Reset audio decoder, especially for audio switch
*
* @param[in]  pcodec  Pointer of codec parameter structure
*
* @return     0 for success, or fail type if < 0
*/
/* --------------------------------------------------------------------------*/
int codec_reset_audio(codec_para_t *pcodec)
{
    return codec_h_ioctl(pcodec->handle, AMSTREAM_IOC_SET, AMSTREAM_AUDIO_RESET, 0);
}

/* --------------------------------------------------------------------------*/
/**
* @brief  codec_reset_subtile   Reset subtitle device, especially for subtitle swith
*
* @param[in]  pcodec  Pointer of codec parameter structure
*
* @return     0 for success, or fail type if < 0
*/
/* --------------------------------------------------------------------------*/
int codec_reset_subtile(codec_para_t *pcodec)
{
    return codec_h_ioctl(pcodec->handle, AMSTREAM_IOC_SET, AMSTREAM_SUB_RESET, 0);
}

/* --------------------------------------------------------------------------*/
/**
* @brief  codec_set_audio_id  Set audio pid by codec device
*
* @param[in]  pcodec  Pointer of codec parameter structure
*
* @return     0 for success, or fail type if < 0
*/
/* --------------------------------------------------------------------------*/
int codec_set_audio_pid(codec_para_t *pcodec)
{
    return codec_h_ioctl(pcodec->handle, AMSTREAM_IOC_SET, AMSTREAM_SET_AID, pcodec->audio_pid);
}

/* --------------------------------------------------------------------------*/
/**
* @brief  codec_set_sub_id  Set subtitle pid by codec device
*
* @param[in]  pcodec  Pointer of codec parameter structure
*
* @return     0 for success, or fail type if < 0
*/
/* --------------------------------------------------------------------------*/
int codec_set_sub_id(codec_para_t *pcodec)
{
    return codec_h_ioctl(pcodec->handle, AMSTREAM_IOC_SET, AMSTREAM_SET_SID, pcodec->sub_pid);
}

/* --------------------------------------------------------------------------*/
/**
* @brief  codec_set_sub_type  Set subtitle type by codec device
*
* @param[in]  pcodec  Pointer of codec parameter structure
*
* @return     0 for success, or fail type if < 0
*/
/* --------------------------------------------------------------------------*/
int codec_set_sub_type(codec_para_t *pcodec)
{
    return codec_h_ioctl(pcodec->handle, AMSTREAM_IOC_SET, AMSTREAM_SET_SUB_TYPE, pcodec->sub_type);
}

/* --------------------------------------------------------------------------*/
/**
* @brief  codec_audio_reinit  Re-initialize audio codec
*
* @param[in]  pcodec  Pointer of codec parameter structure
*
* @return     0 for success, or fail type if < 0
*/
/* --------------------------------------------------------------------------*/
int codec_audio_reinit(codec_para_t *pcodec)
{
	int ret;
	ret = set_audio_format(pcodec);
	if(!ret && pcodec->audio_info.valid){
		ret = set_audio_info(pcodec);
	}
    return ret;
}

/* --------------------------------------------------------------------------*/
/**
* @brief  codec_set_dec_reset  Set decoder reset flag when reset
*
* @param[in]  pcodec  Pointer of codec parameter structure
*
* @return     0 for success, or fail type if < 0
*/
/* --------------------------------------------------------------------------*/
int codec_set_dec_reset(codec_para_t *pcodec)
{
    return codec_h_ioctl(pcodec->handle, AMSTREAM_IOC_SET, AMSTREAM_DEC_RESET, 0);
}

/* --------------------------------------------------------------------------*/
/**
* @brief  codec_audio_isready  check audio finish init ok
*
* @param[in]  pcodec  Pointer of codec parameter structure
*
* @return     1 for ready, or not ready if = 0
*/
/* --------------------------------------------------------------------------*/
int codec_audio_isready(codec_para_t *p)
{
    int audio_isready = 1;
    if (!p) {
        CODEC_PRINT("[%s]ERROR invalid pointer!\n", __FUNCTION__);
        return -1;
    }
    if (p->has_audio) {
        audio_isready = audio_dec_ready(p->adec_priv);
    }

    return audio_isready;
}

/* --------------------------------------------------------------------------*/
/**
* @brief  codec_audio_get_nb_frames  get audiodsp decoded frame number
*
* @param[in]  pcodec  Pointer of codec parameter structure
*
* @return     n decoded frames number, or return 0
*/
/* --------------------------------------------------------------------------*/
int codec_audio_get_nb_frames(codec_para_t *p)
{
    int audio_nb_frames = -1;
    if (!p) {
        CODEC_PRINT("[%s]ERROR invalid pointer!\n", __FUNCTION__);
        return -1;
    }
	
    if (p->has_audio) {
        audio_nb_frames = audio_get_decoded_nb_frames(p->adec_priv);		
    }
	//CODEC_PRINT("[%s]get audio decoded frame number[%d]!\n", __FUNCTION__, audio_nb_frames);
    return audio_nb_frames;
}

/* --------------------------------------------------------------------------*/
/**
* @brief  codec_get_apts  get audio pts
*
* @param[in]  pcodec  Pointer of codec parameter structure
*
* @return     audio pts, or -1 if it failed
*/
/* --------------------------------------------------------------------------*/
int codec_get_apts(codec_para_t *pcodec)
{
    unsigned int apts;
    int ret;
    
    if (!pcodec) {
        CODEC_PRINT("[%s]ERROR invalid pointer!\n", __FUNCTION__);
        return -1;
    }

    ret = codec_h_ioctl(pcodec->handle, AMSTREAM_IOC_GET, AMSTREAM_GET_APTS, (unsigned long)&apts);
    if (ret < 0) {
        CODEC_PRINT("[%s]ioctl failed %d\n", __FUNCTION__, ret);
        return -1;
    }

    return apts;
}

/* --------------------------------------------------------------------------*/
/**
* @brief  codec_get_vpts  get video pts
*
* @param[in]  pcodec  Pointer of codec parameter structure
*
* @return     video pts, or -1 if it failed
*/
/* --------------------------------------------------------------------------*/
int codec_get_vpts(codec_para_t *pcodec)
{
    unsigned int vpts;
    int ret;
    
    if (!pcodec) {
        CODEC_PRINT("[%s]ERROR invalid pointer!\n", __FUNCTION__);
        return -1;
    }

    ret = codec_h_ioctl(pcodec->handle, AMSTREAM_IOC_GET, AMSTREAM_GET_VPTS, (unsigned long)&vpts);
    if (ret < 0) {
        CODEC_PRINT("[%s]ioctl failed %d\n", __FUNCTION__, ret);
        return -1;
    }

    return vpts;
}

/* --------------------------------------------------------------------------*/
/**
* @brief  codec_get_pcrscr  get system pcrscr
*
* @param[in]  pcodec  Pointer of codec parameter structure
*
* @return     system pcrscr, or -1 it failed
*/
/* --------------------------------------------------------------------------*/
int codec_get_pcrscr(codec_para_t *pcodec)
{
    unsigned int pcrscr;
    int ret;
    
    if (!pcodec) {
        CODEC_PRINT("[%s]ERROR invalid pointer!\n", __FUNCTION__);
        return -1;
    }

    ret = codec_h_ioctl(pcodec->handle, AMSTREAM_IOC_GET, AMSTREAM_GET_PCRSCR, (unsigned long)&pcrscr);
    if (ret < 0) {
        CODEC_PRINT("[%s]ioctl failed %d\n", __FUNCTION__, ret);
        return -1;
    }

    return pcrscr;
}

/* --------------------------------------------------------------------------*/
/**
* @brief  codec_set_pcrscr  set system pcrscr
*
* @param[in]  pcodec  Pointer of codec parameter structure
* @param[in]  val  system pcrscr value
*
* @return     0 is success , or -1 failed.
*/
/* --------------------------------------------------------------------------*/
int codec_set_pcrscr(codec_para_t *pcodec, int val)
{
    unsigned int pcrscr;
    int ret;
    
    if (!pcodec) {
        CODEC_PRINT("[%s]ERROR invalid pointer!\n", __FUNCTION__);
        return -1;
    }

    ret = codec_h_ioctl(pcodec->handle, AMSTREAM_IOC_SET, AMSTREAM_SET_PCRSCR, val);
    if (ret < 0) {
        CODEC_PRINT("[%s]ioctl failed %d\n", __FUNCTION__, ret);
        return -1;
    }

    return 0;
}

/* --------------------------------------------------------------------------*/
/**
* @brief  codec_set_syncenable  enable or disable av sync
*
* @param[in]  pcodec  Pointer of codec parameter structure
* @param[in]  enable  Enable or disable to be set
*
* @return     0 for success, or fail type if < 0
*/
/* --------------------------------------------------------------------------*/
int codec_set_syncenable(codec_para_t *pcodec, int enable)
{
    return codec_h_control(pcodec->cntl_handle, AMSTREAM_IOC_SYNCENABLE, (unsigned long)enable);
}

/* --------------------------------------------------------------------------*/
/**
* @brief  codec_set_sync_audio_discont  set sync discontinue state
*
* @param[in]  pcodec       Pointer of codec parameter structure
* @param[in]  discontinue  Discontinue state to be set
*
* @return     0 for success, or fail type if < 0
*/
/* --------------------------------------------------------------------------*/
int codec_set_sync_audio_discont(codec_para_t *pcodec, int discontinue)
{
    return codec_h_control(pcodec->cntl_handle, AMSTREAM_IOC_SET_SYNC_ADISCON, (unsigned long)discontinue);
}

/* --------------------------------------------------------------------------*/
/**
* @brief  codec_set_sync_video_discont  set sync discontinue state
*
* @param[in]  pcodec       Pointer of codec parameter structure
* @param[in]  discontinue  Discontinue state to be set
*
* @return     0 for success, or fail type if < 0
*/
/* --------------------------------------------------------------------------*/
int codec_set_sync_video_discont(codec_para_t *pcodec, int discontinue)
{
    return codec_h_control(pcodec->cntl_handle, AMSTREAM_IOC_SET_SYNC_VDISCON, (unsigned long)discontinue);
}

/* --------------------------------------------------------------------------*/
/**
* @brief  codec_get_sync_audio_discont  get audio sync discontinue state
*
* @param[in]  pcodec       Pointer of codec parameter structure
*
* @return     discontiue state, or fail if < 0
*/
/* --------------------------------------------------------------------------*/
int codec_get_sync_audio_discont(codec_para_t *pcodec)
{
    int discontinue = 0;
    int ret;

    ret = codec_h_control(pcodec->cntl_handle, AMSTREAM_IOC_GET_SYNC_ADISCON, (unsigned long)&discontinue);
    if (ret < 0) {
        return ret;
    }	
    return discontinue;   
}

/* --------------------------------------------------------------------------*/
/**
* @brief  codec_get_sync_video_discont  get video sync discontinue state
*
* @param[in]  pcodec       Pointer of codec parameter structure
*
* @return     discontiue state, or fail if < 0
*/
/* --------------------------------------------------------------------------*/
int codec_get_sync_video_discont(codec_para_t *pcodec)
{
    int discontinue = 0;
    int ret;

    ret = codec_h_control(pcodec->cntl_handle, AMSTREAM_IOC_GET_SYNC_VDISCON, (unsigned long)&discontinue);
    if (ret < 0) {
        return ret;
    }	
    return discontinue;   
}

/* --------------------------------------------------------------------------*/
/**
* @brief  codec_get_sync_audio_discont_diff  get audio sync discontinue state
*
* @param[in]  pcodec       Pointer of codec parameter structure
*
* @return     discontiue diff, or fail if < 0
*/
/* --------------------------------------------------------------------------*/
unsigned long codec_get_sync_audio_discont_diff(codec_para_t *pcodec)
{
    unsigned long discontinue_diff = 0;
    int ret;

    ret = codec_h_control(pcodec->cntl_handle, AMSTREAM_IOC_GET_SYNC_ADISCON_DIFF, (unsigned long)&discontinue_diff);
    if (ret < 0) {
        return ret;
    }	
    return discontinue_diff;   
}

/* --------------------------------------------------------------------------*/
/**
* @brief  codec_get_sync_video_discont_diff  get audio sync discontinue state
*
* @param[in]  pcodec       Pointer of codec parameter structure
*
* @return     discontiue diff, or fail if < 0
*/
/* --------------------------------------------------------------------------*/
unsigned long codec_get_sync_video_discont_diff(codec_para_t *pcodec)
{
    unsigned long discontinue_diff = 0;
    int ret;

    ret = codec_h_control(pcodec->cntl_handle, AMSTREAM_IOC_GET_SYNC_VDISCON_DIFF, (unsigned long)&discontinue_diff);
    if (ret < 0) {
        return ret;
    }	
    return discontinue_diff;   
}

/* --------------------------------------------------------------------------*/
/**
* @brief  codec_set_sync_audio_discont_diff  set sync discontinue diff
*
* @param[in]  pcodec       Pointer of codec parameter structure
* @param[in]  discontinue_diff  Discontinue diff to be set
*
* @return     0 for success, or fail type if < 0
*/
/* --------------------------------------------------------------------------*/
int codec_set_sync_audio_discont_diff(codec_para_t *pcodec, unsigned long discontinue_diff)
{
    return codec_h_control(pcodec->cntl_handle, AMSTREAM_IOC_SET_SYNC_ADISCON_DIFF, discontinue_diff);
}

/* --------------------------------------------------------------------------*/
/**
* @brief  codec_set_sync_video_discont_diff  set sync discontinue diff
*
* @param[in]  pcodec       Pointer of codec parameter structure
* @param[in]  discontinue_diff  Discontinue diff to be set
*
* @return     0 for success, or fail type if < 0
*/
/* --------------------------------------------------------------------------*/
int codec_set_sync_video_discont_diff(codec_para_t *pcodec, unsigned long discontinue_diff)
{
    return codec_h_control(pcodec->cntl_handle, AMSTREAM_IOC_SET_SYNC_VDISCON_DIFF, discontinue_diff);
}

/* --------------------------------------------------------------------------*/
/**
* @brief  codec_get_sub_num  get the number of subtitle
*
* @param[in]  pcodec       Pointer of codec parameter structure
*
* @return     the number of subtitle, or fail if < 0
*/
/* --------------------------------------------------------------------------*/
int codec_get_sub_num(codec_para_t *pcodec)
{
    int sub_num = 0;
    int ret;

    ret = codec_h_ioctl(pcodec->handle, AMSTREAM_IOC_GET, AMSTREAM_GET_SUB_NUM, (unsigned long)&sub_num);
    if (ret < 0) {
        return ret;
    }
    return sub_num;
}

/* --------------------------------------------------------------------------*/
/**
* @brief  codec_get_sub_info  get subtitle information
*
* @param[in]   pcodec       Pointer of codec parameter structure
* @param[out]  sub_info     Pointer of subtitle_info_t to save the subtitle information
*
* @return     0 for success, or fail type if < 0
*/
/* --------------------------------------------------------------------------*/
int codec_get_sub_info(codec_para_t *pcodec, subtitle_info_t *sub_info)
{    
    int ret = 0;
    int i;
    if (!sub_info) {
        CODEC_PRINT("[codec_get_sub_info] error, NULL pointer!\n");
        ret = CODEC_ERROR_INVAL;
        return ret;
    }
    ret = codec_h_ioctl(pcodec->handle, AMSTREAM_IOC_GET_PTR, AMSTREAM_GET_PTR_SUB_INFO, (unsigned long)sub_info);
    if (ret < 0) {
        return ret;
    }
    return ret;
}

/********************************************************************************
*
*the interface for av sync threshold setting
*
*********************************************************************************/
int codec_set_av_threshold(codec_para_t *pcodec, int threshold)
{
    int ret = 0;
    if (pcodec->has_audio)
    {
        audio_set_av_sync_threshold(pcodec->adec_priv, threshold);
    }
    else
    {
        CODEC_PRINT("[codec_set_av_threshold] error, no audio!\n");
        ret = -1;
    }
    
    return ret;
}

/* --------------------------------------------------------------------------*/
/**
* @brief  codec_get_freerun_mode  Get the mode of video freerun
*
* @param[in]  pcodec  Pointer of codec parameter structure
*
* @return     Video free run mode or fail error type
*/
/* --------------------------------------------------------------------------*/
int codec_get_freerun_mode(codec_para_t *pcodec)
{
    int freerun_mode, r;

    if (pcodec->cntl_handle == 0) {
        CODEC_PRINT("no control handler\n");
        return 0;
    }

    r = codec_h_control(pcodec->cntl_handle, AMSTREAM_IOC_GET_FREERUN_MODE, (unsigned long)&freerun_mode);
    if (r < 0) {
        return system_error_to_codec_error(r);
    } else {
        return freerun_mode;
    }
}

/* --------------------------------------------------------------------------*/
/**
* @brief  codec_set_freerun_mode  Set the mode to video freerun
*
* @param[in]  pcodec  Pointer of codec parameter structure
* @param[in]  mode    Freerun mode to be set
*
* @return     0 for success, or fail type if < 0
*/
/* --------------------------------------------------------------------------*/
int codec_set_freerun_mode(codec_para_t *pcodec, unsigned int mode)
{
    return codec_h_control(pcodec->cntl_handle, AMSTREAM_IOC_SET_FREERUN_MODE, (unsigned long)mode);
}

/* --------------------------------------------------------------------------*/
/**
* @brief  codec_init_audio_utils  Initialize the audio utils device
*
* @param[in]  pcodec  Pointer of codec parameter structure
*
* @return     Success or fail error type
*/
/* --------------------------------------------------------------------------*/
int codec_init_audio_utils(codec_para_t *pcodec)
{
    CODEC_HANDLE audio_utils;

    audio_utils = codec_h_open(CODEC_AUDIO_UTILS_DEVICE, O_RDONLY);
    if (audio_utils < 0) {
        CODEC_PRINT("get %s failed\n", CODEC_AUDIO_UTILS_DEVICE);
        return system_error_to_codec_error(audio_utils);
    }

    pcodec->audio_utils_handle = audio_utils;
    
    return CODEC_ERROR_NONE;
}

/* --------------------------------------------------------------------------*/
/**
* @brief  codec_release_audio_utils  Release the audio utils device
*
* @param[in]  pcodec  Pointer of codec parameter structure
*
* @return     Success or fail error type
*/
/* --------------------------------------------------------------------------*/
int codec_release_audio_utils(codec_para_t *pcodec)
{
    if (pcodec) {
        if(pcodec->audio_utils_handle>=0)
            codec_h_close(pcodec->audio_utils_handle);
    }

    pcodec->audio_utils_handle = -1;
    
    return CODEC_ERROR_NONE;
}

/* --------------------------------------------------------------------------*/
/**
* @brief  codec_set_audio_resample_ena  Set audio resample
*
* @param[in]  pcodec  Pointer of codec parameter structure
*
* @return     Success or fail error type
*/
/* --------------------------------------------------------------------------*/
int codec_set_audio_resample_ena(codec_para_t *pcodec, unsigned long mode)
{
    return codec_h_control(pcodec->audio_utils_handle, AMAUDIO_IOC_SET_RESAMPLE_ENA, mode);
}

/* --------------------------------------------------------------------------*/
/**
* @brief  codec_set_audio_resample_ena  Set audio resample enable
*
* @param[in]  pcodec  Pointer of codec parameter structure
*
* @return     Success or fail error type
*/
/* --------------------------------------------------------------------------*/
int codec_get_audio_resample_ena(codec_para_t *pcodec)
{
    unsigned long audio_resample_ena;
    int ret;
    ret = codec_h_control(pcodec->audio_utils_handle, AMAUDIO_IOC_GET_RESAMPLE_ENA, &audio_resample_ena);
    if (ret < 0) {
        return system_error_to_codec_error(ret);
    } else {
        return audio_resample_ena;
    }
}

/* --------------------------------------------------------------------------*/
/**
* @brief  codec_set_audio_resample_type  Set audio resample type
*
* @param[in]  pcodec  Pointer of codec parameter structure
*
* @return     Success or fail error type
*/
/* --------------------------------------------------------------------------*/
int codec_set_audio_resample_type(codec_para_t *pcodec, unsigned long type)
{
    return codec_h_control(pcodec->audio_utils_handle, AMAUDIO_IOC_SET_RESAMPLE_TYPE, type);
}

/* --------------------------------------------------------------------------*/
/**
* @brief  codec_set_video_delay_limited_ms  Set video buffer max delayed,if> settings,write may wait& again,
*
* @param[in]  pcodec  Pointer of codec parameter structure
*
* @return     0 for success, or fail type if < 0
*/
/* --------------------------------------------------------------------------*/
int codec_set_video_delay_limited_ms(codec_para_t *pcodec,int delay_ms)
{
    return codec_h_ioctl(pcodec->handle, AMSTREAM_IOC_SET, AMSTREAM_SET_VIDEO_DELAY_LIMIT_MS, delay_ms);
}
/* --------------------------------------------------------------------------*/
/**
* @brief  codec_get_video_delay_limited_ms  Set video buffer max delayed,if> settings,write may wait& again,
*
* @param[in]  pcodec  Pointer of codec parameter structure
*
* @return     0 for success, or fail type if < 0
*/
/* --------------------------------------------------------------------------*/
int codec_get_video_delay_limited_ms(codec_para_t *pcodec,int *delay_ms)
{
    return codec_h_ioctl(pcodec->handle, AMSTREAM_IOC_GET, AMSTREAM_GET_VIDEO_DELAY_LIMIT_MS, delay_ms);
}


/* --------------------------------------------------------------------------*/
/**
* @brief  codec_set_video_delay_limited_ms  Set video buffer max delayed,if> settings,write may wait& again,
*
* @param[in]  pcodec  Pointer of codec parameter structure
*
* @return     0 for success, or fail type if < 0
*/
/* --------------------------------------------------------------------------*/
int codec_set_audio_delay_limited_ms(codec_para_t *pcodec,int delay_ms)
{
    return codec_h_ioctl(pcodec->handle, AMSTREAM_IOC_SET, AMSTREAM_SET_AUDIO_DELAY_LIMIT_MS, delay_ms);
}

/* --------------------------------------------------------------------------*/
/**
* @brief  codec_get_audio_delay_limited_ms  get video buffer max delayed,if> settings,write may wait& again,
*
* @param[in]  pcodec  Pointer of codec parameter structure
*
* @return     0 for success, or fail type if < 0
*/
/* --------------------------------------------------------------------------*/
int codec_get_audio_delay_limited_ms(codec_para_t *pcodec,int *delay_ms)
{
    return codec_h_ioctl(pcodec->handle, AMSTREAM_IOC_GET, AMSTREAM_GET_AUDIO_DELAY_LIMIT_MS, delay_ms);
}

/* --------------------------------------------------------------------------*/
/**
* @brief  codec_get_audio_cur_delay_ms  get current audio delay ms
*
* @param[in]  pcodec  Pointer of codec parameter structure
*
* @return     0 for success, or fail type if < 0
*/
/* --------------------------------------------------------------------------*/
int codec_get_audio_cur_delay_ms(codec_para_t *pcodec,int *delay_ms)
{
    return codec_h_control(pcodec->handle, AMSTREAM_IOC_GET_AUDIO_CUR_DELAY_MS, delay_ms);
}


/* --------------------------------------------------------------------------*/
/**
* @brief  codec_get_video_cur_delay_ms  get video  current delay ms
*
* @param[in]  pcodec  Pointer of codec parameter structure
*
* @return     0 for success, or fail type if < 0
*/
/* --------------------------------------------------------------------------*/
int codec_get_video_cur_delay_ms(codec_para_t *pcodec,int *delay_ms)
{
    return codec_h_ioctl(pcodec->handle, AMSTREAM_IOC_GET, AMSTREAM_GET_VIDEO_CUR_DELAY_MS, delay_ms);
}

/* --------------------------------------------------------------------------*/
/**
* @brief  codec_get_audio_cur_delay_ms   get vido   latest bitrate.
*
* @param[in]  pcodec  Pointer of codec parameter structure
*
* @return     0 for success, or fail type if < 0
*/
/* --------------------------------------------------------------------------*/
int codec_get_video_cur_bitrate(codec_para_t *pcodec,int *bitrate)
{
    return codec_h_ioctl(pcodec->handle, AMSTREAM_IOC_GET, AMSTREAM_GET_VIDEO_AVG_BITRATE_BPS, bitrate);
}


/* --------------------------------------------------------------------------*/
/**
* @brief  codec_get_audio_cur_bitrate  get audio  latest bitrate.
*
* @param[in]  pcodec  Pointer of codec parameter structure
*
* @return     0 for success, or fail type if < 0
*/
/* --------------------------------------------------------------------------*/
int codec_get_audio_cur_bitrate(codec_para_t *pcodec,int *bitrate)
{
    return codec_h_ioctl(pcodec->handle, AMSTREAM_IOC_GET, AMSTREAM_GET_AUDIO_AVG_BITRATE_BPS, bitrate);
}

/* --------------------------------------------------------------------------*/
/**
* @brief  codec_set_vsync_upint  Set the mode to video freerun
*
* @param[in]  pcodec  Pointer of codec parameter structure
* @param[in]  mode    vsync upint mode to be set
*
* @return     0 for success, or fail type if < 0
*/
/* --------------------------------------------------------------------------*/
int codec_set_vsync_upint(codec_para_t *pcodec, unsigned int mode)
{
    return codec_h_control(pcodec->cntl_handle, AMSTREAM_IOC_SET_VSYNC_UPINT, (unsigned long)mode);
}
int codec_set_drmmode(codec_para_t *pcodec)
{
    return codec_h_ioctl(pcodec->handle, AMSTREAM_IOC_SET, AMSTREAM_SET_DRMMODE, 0);
}

/**
 *
 *
 */
int codec_get_last_checkout_apts(codec_para_t* pcodec, unsigned long *apts)
{
    return codec_h_ioctl(pcodec->handle, AMSTREAM_IOC_GET, AMSTREAM_GET_LAST_CHECKOUT_APTS, apts);
}

int codec_get_last_checkin_apts(codec_para_t* pcodec, unsigned long* apts)
{
    return codec_h_ioctl(pcodec->handle, AMSTREAM_IOC_GET, AMSTREAM_GET_LAST_CHECKIN_APTS, apts);
}

/**
 *@brief  codec_get_pcm_level get the PCM data in buffer between DSP and output
 *
 * @param[in] pcodec Pointer of codec parameter structre
 * @param[in] level  Address to store "level"
 * @return 0 for success, or fail type if<0
 */

int codec_get_pcm_level(codec_para_t* pcodec, unsigned int* level)
{
  return audio_get_pcm_level(pcodec->adec_priv);
}

int codec_set_skip_bytes(codec_para_t* pcodec, unsigned int bytes)
{
  return audio_set_skip_bytes(pcodec->adec_priv);
}

int codec_get_dsp_apts(codec_para_t* pcodec, unsigned int * apts)
{
  return audio_get_pts(pcodec->adec_priv, apts);
}
/*add for gstreamer fast/slow forward*/
int codec_set_video_playrate(codec_para_t* pcodec, int rate)
{
    return codec_h_control(pcodec->cntl_handle, AMSTREAM_IOC_SET_PLAYRATE, (unsigned long)rate);
}
