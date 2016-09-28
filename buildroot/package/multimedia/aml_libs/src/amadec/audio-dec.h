/**
 * \file audio-dec.h
 * \brief  Definitiond Of Audio Dec Types And Structures
 * \version 1.0.0
 * \date 2011-03-08
 */
/* Copyright (C) 2007-2011, Amlogic Inc.
 * All right reserved
 *
 */
#ifndef AUDIO_DEC_H
#define AUDIO_DEC_H

#include<pthread.h>

#include <audio-out.h>
#include <audiodsp.h>
#include <adec-types.h>
#include <adec-message.h>
#include <log-print.h>
#include <adec-armdec-mgt.h>
#include <adec_write.h>
ADEC_BEGIN_DECLS

#define  AUDIO_CTRL_DEVICE    "/dev/amaudio_ctl"

#define AMAUDIO_IOC_MAGIC  'A'
#define AMAUDIO_IOC_SET_LEFT_MONO               _IOW(AMAUDIO_IOC_MAGIC, 0x0e, int)
#define AMAUDIO_IOC_SET_RIGHT_MONO              _IOW(AMAUDIO_IOC_MAGIC, 0x0f, int)
#define AMAUDIO_IOC_SET_STEREO               _IOW(AMAUDIO_IOC_MAGIC, 0x10, int)
#define AMAUDIO_IOC_SET_CHANNEL_SWAP            _IOW(AMAUDIO_IOC_MAGIC, 0x11, int)

//should in accordance with ../amcodec/include/amports/amstream.h
#define AMAUDIO_IOC_SET_RESAMPLE_DELTA        _IOW(AMAUDIO_IOC_MAGIC, 0x1d, unsigned long)

//for ffmpeg audio decode
#define AMSTREAM_IOC_MAGIC  'S'
#define AMSTREAM_IOC_APTS_LOOKUP    _IOR((AMSTREAM_IOC_MAGIC), 0x81, int)  
#define GET_FIRST_APTS_FLAG    _IOR((AMSTREAM_IOC_MAGIC), 0x82, int)
//-----------------------------------------------
//copy from file: "../amcodec/include/amports/amstream.h"
#ifndef AMSTREAM_IOC_PCRSCR
#define AMSTREAM_IOC_PCRSCR  _IOR((AMSTREAM_IOC_MAGIC), 0x42, int)
#endif
#ifndef AMSTREAM_IOC_SET_APTS
#define AMSTREAM_IOC_SET_APTS  _IOW((AMSTREAM_IOC_MAGIC), 0xa8, int)
#endif

//-----------------------------------------------


/*******************************************************************************************/

typedef struct aml_audio_dec    aml_audio_dec_t;
#define DECODE_ERR_PATH "/sys/class/audiodsp/codec_fatal_err"
#define DECODE_NONE_ERR 0
#define DECODE_INIT_ERR 1
#define DECODE_FATAL_ERR 2
#define lock_t            pthread_mutex_t
#define lp_lock_init(x,v)     pthread_mutex_init(x,v)
#define lp_lock(x)        pthread_mutex_lock(x)
#define lp_unlock(x)       pthread_mutex_unlock(x)
typedef enum {
    HW_STEREO_MODE = 0,
    HW_LEFT_CHANNEL_MONO,
    HW_RIGHT_CHANNEL_MONO,
    HW_CHANNELS_SWAP,
} hw_command_t;
struct package{
    char *data;//buf ptr
    int size;               //package size
    struct package * next;//next ptr
};

typedef struct {
    struct package *first;
    int pack_num;
    struct package *current;
    lock_t tslock;
}Package_List;

typedef struct {
    char buff[10];
    int size;
    int status;//0 init 1 finding sync word 2 finding framesize 3 frame size found
}StartCode;
typedef void (*fp_arm_omx_codec_init)(aml_audio_dec_t*,int,void*,int*);
typedef void (*fp_arm_omx_codec_read)(aml_audio_dec_t*,unsigned char *,unsigned *,int *);
typedef void (*fp_arm_omx_codec_close)(aml_audio_dec_t*);
typedef void (*fp_arm_omx_codec_start)(aml_audio_dec_t*);
typedef void (*fp_arm_omx_codec_pause)(aml_audio_dec_t*);
typedef int  (*fp_arm_omx_codec_get_declen)(aml_audio_dec_t*);
typedef int  (*fp_arm_omx_codec_get_FS)(aml_audio_dec_t*);
typedef int  (*fp_arm_omx_codec_get_Nch)(aml_audio_dec_t*);

struct aml_audio_dec {
    adec_state_t  state;
    pthread_t       thread_pid;
    adec_audio_format_t  format;
    int channels;
    int samplerate;
    int data_width;
    int bitrate;
    int block_align;
    int codec_id;
    int need_stop;
    int auto_mute;
    int muted;
    int decoded_nb_frames;
    int avsync_threshold;
    float volume; //left or main volume
    float volume_ext; //right	
    //codec_para_t *pcodec;
    hw_command_t soundtrack;
    audio_out_operations_t aout_ops;
    dsp_operations_t adsp_ops;
    message_pool_t message_pool;
    audio_decoder_operations_t *adec_ops;//non audiodsp decoder operations
    int extradata_size;      ///< extra data size
    char extradata[AUDIO_EXTRA_DATA_SIZE];
	int SessionID;
	int format_changed_flag;
	unsigned dspdec_not_supported;//check some profile that audiodsp decoder can not support,we switch to arm decoder	
	int droppcm_flag;				// drop pcm flag, if switch audio (1)
	int no_first_apts;				// if can't get the first apts (1), default (0)
	int StageFrightCodecEnableType;
	int64_t pcm_bytes_readed;
	int64_t raw_bytes_readed;
	int codec_type;
	int raw_frame_size;
	int pcm_frame_size;
	int i2s_iec958_sync_flag;
	int max_bytes_readded_diff;
    int i2s_iec958_sync_gate;

    buffer_stream_t *g_bst;
    buffer_stream_t *g_bst_raw;
    pthread_t sn_threadid;
    pthread_t sn_getpackage_threadid;
    int exit_decode_thread;
    int exit_decode_thread_success;
    unsigned long decode_offset;
    int nDecodeErrCount;
    int fd_uio;
    int last_valid_pts;
    int out_len_after_last_valid_pts;
    int pcm_cache_size;
    Package_List pack_list;
    StartCode start_code;
    
    void *arm_omx_codec;
    fp_arm_omx_codec_init       parm_omx_codec_init;
    fp_arm_omx_codec_read       parm_omx_codec_read ;
    fp_arm_omx_codec_close      parm_omx_codec_close;
    fp_arm_omx_codec_start      parm_omx_codec_start;
    fp_arm_omx_codec_pause      parm_omx_codec_pause;
    fp_arm_omx_codec_get_declen parm_omx_codec_get_declen;
    fp_arm_omx_codec_get_FS     parm_omx_codec_get_FS;
    fp_arm_omx_codec_get_Nch    parm_omx_codec_get_Nch;
    int OmxFirstFrameDecoded;
};

//from amcodec
typedef struct {
    int sample_rate;         ///< audio stream sample rate
    int channels;            ///< audio stream channels
    int format;            ///< codec format id
    int bitrate;
    int block_align;
    int codec_id;         //original codecid corespingding to ffmepg
    int handle;        ///< codec device handler
    int extradata_size;      ///< extra data size
    char extradata[AUDIO_EXTRA_DATA_SIZE];
	int SessionID;
	int dspdec_not_supported;//check some profile that audiodsp decoder can not support,we switch to arm decoder	
	int droppcm_flag;				// drop pcm flag, if switch audio (1)
} arm_audio_info;

 typedef struct {
    int valid;               ///< audio extradata valid(1) or invalid(0), set by dsp
    int sample_rate;         ///< audio stream sample rate
    int channels;            ///< audio stream channels
    int bitrate;             ///< audio stream bit rate
    int codec_id;            ///< codec format id
    int block_align;         ///< audio block align from ffmpeg
    int extradata_size;      ///< extra data size
    char extradata[512];;   ///< extra data information for decoder
} Asf_audio_info_t;

//status check
struct adec_status {
    unsigned int channels;
    unsigned int sample_rate;
    unsigned int resolution;
    unsigned int error_count;
    unsigned int status;
};

//audio decoder type, default arc
#define AUDIO_ARC_DECODER 0
#define AUDIO_ARM_DECODER 1
#define AUDIO_FFMPEG_DECODER 2

//reference from " /amcodec/include/amports/aformat.h"
#define    ACODEC_FMT_NULL   -1
#define    ACODEC_FMT_MPEG   0
#define    ACODEC_FMT_PCM_S16LE  1
#define    ACODEC_FMT_AAC   2
#define    ACODEC_FMT_AC3    3
#define    ACODEC_FMT_ALAW  4
#define    ACODEC_FMT_MULAW  5
#define    ACODEC_FMT_DTS  6
#define    ACODEC_FMT_PCM_S16BE  7
#define    ACODEC_FMT_FLAC  8
#define    ACODEC_FMT_COOK  9
#define    ACODEC_FMT_PCM_U8  10
#define    ACODEC_FMT_ADPCM  11
#define    ACODEC_FMT_AMR   12
#define    ACODEC_FMT_RAAC   13
#define    ACODEC_FMT_WMA   14
#define    ACODEC_FMT_WMAPRO    15
#define    ACODEC_FMT_PCM_BLURAY   16
#define    ACODEC_FMT_ALAC   17
#define    ACODEC_FMT_VORBIS     18
#define    ACODEC_FMT_AAC_LATM    19
#define    ACODEC_FMT_APE    20
#define    ACODEC_FMT_EAC3    21 
#define    ACODEC_FMT_WIFIDISPLAY 22


//audio output type

#define    AUDIO_PCM_OUTPUT 0//output pcm data
#define    AUDIO_SPDIF_PASSTHROUGH 1//passthrough type as dd
#define    AUDIO_HDMI_PASSTHROUGH 2//passthrough type as ddplus


/***********************************************************************************************/
extern void android_basic_init(void);
int audiodec_init(aml_audio_dec_t *aml_audio_dec);
int adec_message_pool_init(aml_audio_dec_t *);
adec_cmd_t *adec_message_alloc(void);
int adec_message_free(adec_cmd_t *);
int adec_send_message(aml_audio_dec_t *, adec_cmd_t *);
adec_cmd_t *adec_get_message(aml_audio_dec_t *);
int feeder_init(aml_audio_dec_t *);
int feeder_release(aml_audio_dec_t *);
void *adec_armdec_loop(void *args);
ADEC_END_DECLS
#endif
