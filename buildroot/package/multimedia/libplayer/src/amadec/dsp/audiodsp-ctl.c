/**
 * \file audiodsp-ctl.c
 * \brief  Functions of Auduodsp control
 * \version 1.0.0
 * \date 2011-03-08
 */
/* Copyright (C) 2007-2011, Amlogic Inc.
 * All right reserved
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include <audio-dec.h>
#include <audiodsp.h>
#include <log-print.h>
#include <cutils/properties.h>

firmware_s_t firmware_list[] = {
    {0, MCODEC_FMT_MPEG123, "audiodsp_codec_mad.bin"},
    {1, MCODEC_FMT_AAC, "audiodsp_codec_aac_helix.bin"},
    {2, MCODEC_FMT_AC3 | MCODEC_FMT_EAC3, "audiodsp_codec_ddp_dcv.bin"},
    {3, MCODEC_FMT_DTS, "audiodsp_codec_dtshd.bin"},
    {4, MCODEC_FMT_FLAC, "audiodsp_codec_flac.bin"},
    {5, MCODEC_FMT_COOK, "audiodsp_codec_cook.bin"},
    {6, MCODEC_FMT_AMR, "audiodsp_codec_amr.bin"},
    {7, MCODEC_FMT_RAAC, "audiodsp_codec_raac.bin"},
    {8, MCODEC_FMT_ADPCM, "audiodsp_codec_adpcm.bin"},
    {9, MCODEC_FMT_WMA, "audiodsp_codec_wma.bin"},
    {10, MCODEC_FMT_PCM, "audiodsp_codec_pcm.bin"},
    {11, MCODEC_FMT_WMAPRO, "audiodsp_codec_wmapro.bin"},
    {12, MCODEC_FMT_ALAC, "audiodsp_codec_alac.bin"},
    {13, MCODEC_FMT_VORBIS, "audiodsp_codec_vorbis.bin"},
    {14, MCODEC_FMT_AAC_LATM, "audiodsp_codec_aac.bin"},
    {15, MCODEC_FMT_APE, "audiodsp_codec_ape.bin"},
    
};

/**
 * \brief register audio firmware
 * \param fd dsp handle
 * \param fmt audio format
 * \param name audio firmware name
 * \return 0 on success otherwise negative error code
 */
static int register_firmware(int fd, int fmt, char *name)
{
    int ret;
    audiodsp_cmd_t cmd;

    cmd.cmd = AUDIODSP_REGISTER_FIRMWARE;
    cmd.fmt = fmt;
    cmd.data = name;
    cmd.data_len = strlen(name);

    ret = ioctl(fd, AUDIODSP_REGISTER_FIRMWARE, &cmd);

    return ret;
}

/**
 * \brief switch audio format
 * \param fmt audio format
 * \return new format on success otherwise zero
 */
static int switch_audiodsp(adec_audio_format_t fmt)
{
    switch (fmt) {
    case  ADEC_AUDIO_FORMAT_MPEG:
        return MCODEC_FMT_MPEG123;

    case  ADEC_AUDIO_FORMAT_AAC_LATM:
        return MCODEC_FMT_AAC_LATM;

    case  ADEC_AUDIO_FORMAT_AAC:
        return MCODEC_FMT_AAC;

    case  ADEC_AUDIO_FORMAT_AC3:
        return MCODEC_FMT_AC3;
	case  ADEC_AUDIO_FORMAT_EAC3:
		return MCODEC_FMT_EAC3;

    case  ADEC_AUDIO_FORMAT_DTS:
        return MCODEC_FMT_DTS;

    case  ADEC_AUDIO_FORMAT_FLAC:
        return MCODEC_FMT_FLAC;

    case  ADEC_AUDIO_FORMAT_COOK:
        return MCODEC_FMT_COOK;

    case  ADEC_AUDIO_FORMAT_AMR:
        return MCODEC_FMT_AMR;

    case  ADEC_AUDIO_FORMAT_RAAC:
        return MCODEC_FMT_RAAC;

    case ADEC_AUDIO_FORMAT_ADPCM:
        return MCODEC_FMT_ADPCM;

    case ADEC_AUDIO_FORMAT_PCM_S16BE:
    case ADEC_AUDIO_FORMAT_PCM_S16LE:
    case ADEC_AUDIO_FORMAT_PCM_U8:
    case ADEC_AUDIO_AFORMAT_PCM_BLURAY:
    case ADEC_AUDIO_FORMAT_PCM_WIFIDISPLAY:
        return MCODEC_FMT_PCM;

    case ADEC_AUDIO_FORMAT_WMA:
        return MCODEC_FMT_WMA;

    case  ADEC_AUDIO_FORMAT_WMAPRO:
        return MCODEC_FMT_WMAPRO;
    case  ADEC_AUDIO_AFORMAT_ALAC:
        return MCODEC_FMT_ALAC;
    case  ADEC_AUDIO_AFORMAT_VORBIS:
        return MCODEC_FMT_VORBIS;
    case  ADEC_AUDIO_FORMAT_APE:
        return MCODEC_FMT_APE;
    default:
        return 0;
    }
}

/**
 * \brief find audio firmware through audio format
 * \param fmt audio format
 * \return firmware_s struct otherwise null
 */
static firmware_s_t * find_firmware_by_fmt(int m_fmt)
{
    int i;
    int num;
    firmware_s_t *f;

    num = ARRAY_SIZE(firmware_list);

    for (i = 0; i < num; i++) {
        f = &firmware_list[i];
        if (f->fmt & m_fmt) {
            return f;
        }
    }

    return NULL;
}


/**
 * \brief init audiodsp
 * \param dsp_ops pointer to dsp operation struct
 * \return 0 on success otherwise -1 if an error occurred
 */
int audiodsp_init(dsp_operations_t *dsp_ops)
{
    int i;
    int fd = -1;
    int num;
    int ret;
    firmware_s_t *f;

    num = ARRAY_SIZE(firmware_list);

    if (dsp_ops->dsp_file_fd < 0) {
        fd = open(DSP_DEV_NOD, O_RDONLY, 0644);
    }

    if (fd < 0) {
        adec_print("unable to open audio dsp  %s,err: %s", DSP_DEV_NOD, strerror(errno));
        return -1;
    }
    ioctl(fd, AUDIODSP_UNREGISTER_ALLFIRMWARE, 0);
    for (i = 0; i < num; i++) {
        f = &firmware_list[i];
        ret = register_firmware(fd, f->fmt, f->name);
        if (ret != 0) {
            adec_print("register firmware error=%d,fmt:%d,name:%s\n", ret, f->fmt, f->name);
        }
    }
    if(i>0)
		ret=0;//ignore the some fmt register error,for compatible some old kernel.can't support muti filename,
    if (ret != 0) {
        close(fd);

    }

    dsp_ops->dsp_file_fd = fd;

    return ret;
}

/**
 * \brief start audiodsp
 * \param dsp_ops pointer to dsp operation struct
 * \return 0 on success otherwise negative code error
 */
 static err_count = 0;

#define PARSER_WAIT_MAX 100
int audiodsp_start(aml_audio_dec_t *audec)
{
    int m_fmt;
    int ret = -1;
    unsigned long val;
    dsp_operations_t *dsp_ops = &audec->adsp_ops;

    if (dsp_ops->dsp_file_fd < 0) {
        return -1;
    }

    if (am_getconfig_bool("media.libplayer.wfd")) {
        ioctl(dsp_ops->dsp_file_fd, AUDIODSP_SET_PCM_BUF_SIZE, 8*1024);
    } else {
        ioctl(dsp_ops->dsp_file_fd, AUDIODSP_SET_PCM_BUF_SIZE, 32*1024);
    }
    
    m_fmt = switch_audiodsp(audec->format);
    adec_print("[%s:%d]  audio_fmt=%d\n", __FUNCTION__, __LINE__, m_fmt);

    if (find_firmware_by_fmt(m_fmt) == NULL) {
        return -2;
    }

    ioctl(dsp_ops->dsp_file_fd, AUDIODSP_SET_FMT, m_fmt);

    ret = ioctl(dsp_ops->dsp_file_fd, AUDIODSP_START, 0);
    if (ret != 0) {
        return -3;
    }

    if(audec->need_stop){ //in case  stop command comes now  
        ioctl(dsp_ops->dsp_file_fd, AUDIODSP_STOP, 0);
        return -5;
    }

    ret = ioctl(dsp_ops->dsp_file_fd, AUDIODSP_DECODE_START, 0);
    err_count = 0;
    if(ret==0){
        do{
            ret = ioctl(dsp_ops->dsp_file_fd, AUDIODSP_WAIT_FORMAT, 0);
	    if(ret!=0 && !audec->need_stop){
                err_count++;		
                usleep(1000*20);
                if (err_count > PARSER_WAIT_MAX){ 
	             ioctl(dsp_ops->dsp_file_fd, AUDIODSP_STOP, 0);//audiodsp_start failed,should stop audiodsp 								
	             adec_print("[%s:%d] audio dsp not ready for decode PCM in 2s\n", __FUNCTION__, __LINE__);
                    return -4;	
                    }				
	    }
        }while(!audec->need_stop && (ret!=0));
    }
	
    if (ret != 0) {
	 ioctl(dsp_ops->dsp_file_fd, AUDIODSP_STOP, 0);//audiodsp_start failed,should stop audiodsp 					
        return -4;
    }

    ioctl(dsp_ops->dsp_file_fd, AUDIODSP_GET_CHANNELS_NUM, &val);
    if (val != (unsigned long) - 1) {
        audec->channels = val;
    }

    ioctl(dsp_ops->dsp_file_fd, AUDIODSP_GET_SAMPLERATE, &val);
    if (val != (unsigned long) - 1) {
        audec->samplerate = val;
    }

    ioctl(dsp_ops->dsp_file_fd, AUDIODSP_GET_BITS_PER_SAMPLE, &val);
    if (val != (unsigned long) - 1) {
        audec->data_width = val;
    }
    adec_print("channels == %d, samplerate == %d\n", audec->channels, audec->samplerate);
    return ret;
}

/**
 * \brief stop audiodsp
 * \param dsp_ops pointer to dsp operation struct
 * \return 0 on success otherwise -1 if an error occurred
 */
int audiodsp_stop(dsp_operations_t *dsp_ops)
{
    int ret;

    if (dsp_ops->dsp_file_fd < 0) {
        return -1;
    }

    ret = ioctl(dsp_ops->dsp_file_fd, AUDIODSP_STOP, 0);

    return ret;
}

/**
 * \brief release audiodsp
 * \param dsp_ops pointer to dsp operation struct
 * \return 0 on success otherwise -1 if an error occurred
 */
int audiodsp_release(dsp_operations_t *dsp_ops)
{
    if (dsp_ops->dsp_file_fd < 0) {
        return -1;
    }

    close(dsp_ops->dsp_file_fd);

    dsp_ops->dsp_file_fd = -1;

    return 0;
}

/**
 * \brief read pcm data from audiodsp
 * \param dsp_ops pointer to dsp operation struct
 * \param buffer point to buffer where storing pcm data
 * \param size read data length
 * \return the number of bytes read
 */
int audiodsp_stream_read(dsp_operations_t *dsp_ops, char *buffer, int size)
{
    if (dsp_ops->dsp_file_fd < 0) {
        adec_print("read error!! audiodsp have not opened\n");
        return 0;
    }

    return read(dsp_ops->dsp_file_fd, buffer, size);
}

/**
 * \brief get current audio pts
 * \param dsp_ops pointer to dsp operation struct
 * \return current audio pts otherwise -1 if an error occurred
 */
unsigned long  audiodsp_get_pts(dsp_operations_t *dsp_ops)
{
    unsigned long val;

    if (dsp_ops->dsp_file_fd < 0) {
        adec_print("read error!! audiodsp have not opened\n");
        return -1;
    }

    ioctl(dsp_ops->dsp_file_fd, AUDIODSP_GET_PTS, &val);

    return val;
}

/**
 * \brief get current audio pcrscr
 * \param dsp_ops pointer to dsp operation struct
 * \return current audio pcrscr otherwise -1 if an error occurred
 */
unsigned long  audiodsp_get_pcrscr(dsp_operations_t *dsp_ops)
{
    unsigned long val;

    if (dsp_ops->dsp_file_fd < 0) {
        adec_print("read error!! audiodsp have not opened\n");
        return -1;
    }

    ioctl(dsp_ops->dsp_file_fd, AUDIODSP_SYNC_GET_PCRSCR, &val);

    return val;
}
/**
 * \brief set current audio pts
 * \param dsp_ops pointer to dsp operation struct
 * \return 0 on success otherwise -1 if an error occurred
 */
int   audiodsp_set_apts(dsp_operations_t *dsp_ops,unsigned long apts)
{

    if (dsp_ops->dsp_file_fd < 0) {
        adec_print("read error!! audiodsp have not opened\n");
        return -1;
    }

    ioctl(dsp_ops->dsp_file_fd, AUDIODSP_SYNC_SET_APTS, &apts);

    return 0;
}
/**
 * \brief get decoded audio frame number
 * \param dsp_ops pointer to dsp operation struct
 * \return audiodsp decoded frame number, -1 if an error occurred
 */
int audiodsp_get_decoded_nb_frames(dsp_operations_t *dsp_ops)
{
    int val = -1;
    if (dsp_ops && dsp_ops->dsp_file_fd) {
        ioctl(dsp_ops->dsp_file_fd, AUDIODSP_GET_DECODED_NB_FRAMES, &val);
    }
    //adec_print("get audio decoded frame number==%d\n",val);
    return val;
}

int audiodsp_get_first_pts_flag(dsp_operations_t *dsp_ops)
{
    int val;

    if (dsp_ops->dsp_file_fd < 0) {
        adec_print("read error!! audiodsp have not opened\n");
        return -1;
    }

    ioctl(dsp_ops->dsp_file_fd, AUDIODSP_GET_FIRST_PTS_FLAG, &val);

    return val;
}

int audiodsp_automute_on(dsp_operations_t *dsp_ops)
{
    int ret;

    if (dsp_ops->dsp_file_fd < 0) {
        adec_print("read error!! audiodsp have not opened\n");
        return -1;
    }

    ret = ioctl(dsp_ops->dsp_file_fd, AUDIODSP_AUTOMUTE_ON, 0);

    return ret;
}

int audiodsp_automute_off(dsp_operations_t *dsp_ops)
{
    int ret;

    if (dsp_ops->dsp_file_fd < 0) {
        adec_print("read error!! audiodsp have not opened\n");
        return -1;
    }

    ret = ioctl(dsp_ops->dsp_file_fd, AUDIODSP_AUTOMUTE_OFF, 0);

    return ret;
}
int audiodsp_get_pcm_level(dsp_operations_t* dsp_ops)
{
  int val = 0;
  if(dsp_ops->dsp_file_fd < 0){
    adec_print("read error !! audiodsp have not opened\n");
    return -1;
  }

  ioctl(dsp_ops->dsp_file_fd, AUDIODSP_GET_PCM_LEVEL, &val);
  return val;
}

int audiodsp_set_skip_bytes(dsp_operations_t* dsp_ops, unsigned int bytes)
{
  if(dsp_ops->dsp_file_fd < 0){
    adec_print("read error !! audiodsp have not opened\n");
    return -1;
  }

  return ioctl(dsp_ops->dsp_file_fd, AUDIODSP_SKIP_BYTES, bytes);
}
