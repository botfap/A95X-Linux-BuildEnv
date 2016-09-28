/**
 * \file alsa-out.c
 * \brief  Functions of Auduo output control for Linux Platform
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
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/soundcard.h>
//#include <config.h>
#include <alsa/asoundlib.h>
#include <alsa/pcm.h>
#include <audio-dec.h>
#include <adec-pts-mgt.h>
#include <log-print.h>
#include <alsa-out.h>
#include "alsactl_parser.h"
#include "alsa-out-raw.h"

//#define adec_print printf
#define adec_print

#define   PERIOD_SIZE  1024
#define   PERIOD_NUM    4

static snd_pcm_sframes_t (*readi_func)(snd_pcm_t *handle, void *buffer, snd_pcm_uframes_t size);
static snd_pcm_sframes_t (*writei_func)(snd_pcm_t *handle, const void *buffer, snd_pcm_uframes_t size);
static snd_pcm_sframes_t (*readn_func)(snd_pcm_t *handle, void **bufs, snd_pcm_uframes_t size);
static snd_pcm_sframes_t (*writen_func)(snd_pcm_t *handle, void **bufs, snd_pcm_uframes_t size);


static int fragcount = PERIOD_NUM;
static snd_pcm_uframes_t chunk_size = PERIOD_SIZE;
static char output_buffer[64 * 1024];
static unsigned char decode_buffer[OUTPUT_BUFFER_SIZE + 64];


static int set_params_raw(alsa_param_t *alsa_params);


int alsa_get_aml_card()
{
    int card = -1, err = 0;
    int fd = -1;
    unsigned fileSize = 512;
    char *read_buf = NULL, *pd = NULL;
    static const char *const SOUND_CARDS_PATH = "/proc/asound/cards";
    fd = open(SOUND_CARDS_PATH, O_RDONLY);
    if (fd < 0) {
        adec_print("ERROR: failed to open config file %s error: %d\n", SOUND_CARDS_PATH, errno);
        close(fd);
        return -1;
    }

    read_buf = (char *)malloc(fileSize);
    if (!read_buf) {
        adec_print("Failed to malloc read_buf");
        close(fd);
        return -1;
    }
    memset(read_buf, 0x0, fileSize);
    err = read(fd, read_buf, fileSize);
    if (fd < 0) {
        adec_print("ERROR: failed to read config file %s error: %d\n", SOUND_CARDS_PATH, errno);
        free(read_buf);
        close(fd);
        return -1;
    }
    pd = strstr(read_buf, "AML");
    card = *(pd - 3) - '0';

OUT:
    free(read_buf);
    close(fd);
    return card;
}

int alsa_get_spdif_port()
{
    int port = -1, err = 0;
    int fd = -1;
    unsigned fileSize = 512;
    char *read_buf = NULL, *pd = NULL;
    static const char *const SOUND_PCM_PATH = "/proc/asound/pcm";
    fd = open(SOUND_PCM_PATH, O_RDONLY);
    if (fd < 0) {
        adec_print("ERROR: failed to open config file %s error: %d\n", SOUND_PCM_PATH, errno);
        close(fd);
        return -1;
    }

    read_buf = (char *)malloc(fileSize);
    if (!read_buf) {
        adec_print("Failed to malloc read_buf");
        close(fd);
        return -1;
    }
    memset(read_buf, 0x0, fileSize);
    err = read(fd, read_buf, fileSize);
    if (fd < 0) {
        adec_print("ERROR: failed to read config file %s error: %d\n", SOUND_PCM_PATH, errno);
        free(read_buf);
        close(fd);
        return -1;
    }
    pd = strstr(read_buf, "SPDIF");
    if (!pd) {
        goto OUT;
    }
    adec_print("%s  \n", pd);

    port = *(pd - 3) - '0';
    adec_print("%s  \n", (pd - 3));

OUT:
    free(read_buf);
    close(fd);
    return port;
}

int alsa_swtich_port(alsa_param_t *alsa_params, int card, int port)
{
    char dev[10] = {0};
    adec_print("card = %d, port = %d\n", card, port);
    sprintf(dev, "hw:%d,%d", (card >= 0) ? card : 0, (port >= 0) ? port : 0);
    pthread_mutex_lock(&alsa_params->playback_mutex);
    snd_pcm_drop(alsa_params->handle);
    snd_pcm_close(alsa_params->handle);
    alsa_params->handle = NULL;
    int err = snd_pcm_open(&alsa_params->handle, dev, SND_PCM_STREAM_PLAYBACK, 0);

    if (err < 0) {
        adec_print("audio open error: %s", snd_strerror(err));
        pthread_mutex_unlock(&alsa_params->playback_mutex);
        return -1;
    }

    set_params_raw(alsa_params);
    pthread_mutex_unlock(&alsa_params->playback_mutex);

    return 0;
}


static int set_params_raw(alsa_param_t *alsa_params)
{
    snd_pcm_hw_params_t *hwparams;
    snd_pcm_sw_params_t *swparams;
    //  snd_pcm_uframes_t buffer_size;
    //  snd_pcm_uframes_t boundary;
    //  unsigned int period_time = 0;
    //  unsigned int buffer_time = 0;
    snd_pcm_uframes_t bufsize;
    int err;
    unsigned int rate;
    snd_pcm_uframes_t start_threshold, stop_threshold;
    snd_pcm_hw_params_alloca(&hwparams);
    snd_pcm_sw_params_alloca(&swparams);

    err = snd_pcm_hw_params_any(alsa_params->handle, hwparams);
    if (err < 0) {
        adec_print("Broken configuration for this PCM: no configurations available");
        return err;
    }

    err = snd_pcm_hw_params_set_access(alsa_params->handle, hwparams,
                                       SND_PCM_ACCESS_RW_INTERLEAVED);
    if (err < 0) {
        adec_print("Access type not available");
        return err;
    }

    err = snd_pcm_hw_params_set_format(alsa_params->handle, hwparams, alsa_params->format);
    if (err < 0) {
        adec_print("Sample format non available");
        return err;
    }

    err = snd_pcm_hw_params_set_channels(alsa_params->handle, hwparams, alsa_params->channelcount);
    if (err < 0) {
        adec_print("Channels count non available");
        return err;
    }

    rate = alsa_params->rate;
    err = snd_pcm_hw_params_set_rate_near(alsa_params->handle, hwparams, &alsa_params->rate, 0);
    assert(err >= 0);
#if 0
    err = snd_pcm_hw_params_get_buffer_time_max(hwparams,  &buffer_time, 0);
    assert(err >= 0);
    if (buffer_time > 500000) {
        buffer_time = 500000;
    }

    period_time = buffer_time / 4;

    err = snd_pcm_hw_params_set_period_time_near(handle, hwparams,
            &period_time, 0);
    assert(err >= 0);

    err = snd_pcm_hw_params_set_buffer_time_near(handle, hwparams,
            &buffer_time, 0);
    assert(err >= 0);

#endif
    alsa_params->bits_per_sample = snd_pcm_format_physical_width(alsa_params->format);
    //bits_per_frame = bits_per_sample * hwparams.realchanl;
    alsa_params->bits_per_frame = alsa_params->bits_per_sample * alsa_params->channelcount;

    bufsize = 	PERIOD_NUM*PERIOD_SIZE*4;

    err = snd_pcm_hw_params_set_buffer_size_near(alsa_params->handle, hwparams,&bufsize);
    if (err < 0) {
        adec_print("Unable to set  buffer  size \n");
        return err;
    }
	
    err = snd_pcm_hw_params_set_period_size_near(alsa_params->handle, hwparams, &chunk_size, NULL);
    if (err < 0) {
        adec_print("Unable to set period size \n");
        return err;
    }
#if 0	
    err = snd_pcm_hw_params_set_periods_near(alsa_params->handle, hwparams, &fragcount, NULL);
    if (err < 0) {
      adec_print("Unable to set periods \n");
      return err;
    }
#endif	
    err = snd_pcm_hw_params(alsa_params->handle, hwparams);
    if (err < 0) {
        adec_print("Unable to install hw params:");
        return err;
    }

    err = snd_pcm_hw_params_get_buffer_size(hwparams, &bufsize);
    if (err < 0) {
        adec_print("Unable to get buffersize \n");
        return err;
    }
    adec_print("[%s::%d]--[alsa raw buffer frame size:%d]\n", __FUNCTION__, __LINE__,bufsize);
    alsa_params->buffer_size = bufsize * alsa_params->bits_per_frame / 8;

#if 1
    err = snd_pcm_sw_params_current(alsa_params->handle, swparams);
    if (err < 0) {
        adec_print("??Unable to get sw-parameters\n");
        return err;
    }

    //err = snd_pcm_sw_params_get_boundary(swparams, &boundary);
    //if (err < 0){
    //  adec_print("Unable to get boundary\n");
    //  return err;
    //}

    //err = snd_pcm_sw_params_set_start_threshold(handle, swparams, bufsize);
    //if (err < 0) {
    //  adec_print("Unable to set start threshold \n");
    //  return err;
    //}

    //err = snd_pcm_sw_params_set_stop_threshold(handle, swparams, buffer_size);
    //if (err < 0) {
    //  adec_print("Unable to set stop threshold \n");
    //  return err;
    //}

    //  err = snd_pcm_sw_params_set_silence_size(handle, swparams, buffer_size);
    //  if (err < 0) {
    //      adec_print("Unable to set silence size \n");
    //      return err;
    //  }

    err = snd_pcm_sw_params(alsa_params->handle, swparams);
    if (err < 0) {
        adec_print("Unable to get sw-parameters\n");
        return err;
    }

    //snd_pcm_sw_params_free(swparams);
#endif


    //chunk_bytes = chunk_size * bits_per_frame / 8;

    return 0;
}

static size_t pcm_write_raw(alsa_param_t * alsa_param, u_char * data, size_t count)
{

    snd_pcm_sframes_t r;
    size_t result = 0;

    /*
        if (count < chunk_size) {
            snd_pcm_format_set_silence(hwparams.format, data + count * bits_per_frame / 8, (chunk_size - count) * hwparams.channels);
            count = chunk_size;
        }
    */
    while (count > 0) {		
        r = writei_func(alsa_param->handle, data, count);

        if (r == -EINTR) {
            r = 0;
        }
        if (r == -ESTRPIPE) {
            while ((r = snd_pcm_resume(alsa_param->handle)) == -EAGAIN) {
                sleep(1);
            }
        }

        if (r < 0) {
            //printf("xun in\n");
            if ((r = snd_pcm_prepare(alsa_param->handle)) < 0) {
                return 0;
            }
        }

        if (r > 0) {
            result += r;
            count -= r;
            data += r * alsa_param->bits_per_frame / 8;
        }
    }
    return result;
}


//spdif output data align
static int alsa_play_raw(alsa_param_t * alsa_param, char * data, unsigned len)
{
    size_t l = 0, r;
    if (!alsa_param->flag) {
        l = len * 8 / alsa_param->bits_per_frame;
        l = l & (~(32 - 1)); /*driver only support  32 frames each time */
        r = pcm_write_raw(alsa_param, data, l);
        r = r * alsa_param->bits_per_frame / 8;
    }

    return r ;
}

static void *alsa_playback_raw_loop(void *args)
{

    int len = 0;
    int len2 = 0;
    int offset = 0;
    aml_audio_dec_t *audec;
    alsa_param_t *alsa_params;
    alsa_param_t *alsa_params_pcm;
    unsigned char *buffer = (unsigned char *)(((unsigned long)decode_buffer + 32) & (~0x1f));

    audec = (aml_audio_dec_t *)args;
    alsa_params = (alsa_param_t *)audec->aout_ops.private_data_raw;
 /*   pthread_mutex_init(&alsa_params->playback_mutex, NULL);
    pthread_cond_init(&alsa_params->playback_cond, NULL);*/

    pthread_mutex_lock(&alsa_params->playback_mutex);
    
    while( !alsa_params->wait_flag )	
    {
        adec_print("yvonnepthread_cond_wait\n");
         pthread_cond_wait(&alsa_params->playback_cond, &alsa_params->playback_mutex);
    }
    alsa_params->wait_flag=1;
    pthread_mutex_unlock(&alsa_params->playback_mutex);


    while (!alsa_params->stop_flag) {
        while ((len < (128 * 2)) && (!alsa_params->stop_flag)) {
            if (offset > 0) {
                memcpy(buffer, buffer + offset, len);
            }
            len2 = audec->adsp_ops.dsp_read_raw(&audec->adsp_ops, (buffer + len), (OUTPUT_BUFFER_SIZE - len));
            len = len + len2;
            offset = 0;
        }

        while (alsa_params->pause_flag) {
            usleep(10000);
        }

        len2 = alsa_play_raw(alsa_params, (buffer + offset), len);

        if (len2 >= 0) {
            len -= len2;
            offset += len2;
        } else {
            len = 0;
            offset = 0;
        }

    }

    adec_print("Exit alsa playback raw loop !\n");
    pthread_exit(NULL);
    return NULL;
}

/**
 * \brief output initialization
 * \param audec pointer to audec
 * \return 0 on success otherwise negative error code
 */
int alsa_init_raw(struct aml_audio_dec* audec)
{

    int err = 0;
    pthread_t tid;
    alsa_param_t *alsa_param;
    audio_out_operations_t *out_ops = &audec->aout_ops;
    int sound_card_id = 0;
    int sound_dev_id = 0;
    char sound_card_dev[10] = {0};

    int dgraw = amsysfs_get_sysfs_int("/sys/class/audiodsp/digital_raw");

    if(AUDIO_SPDIF_PASSTHROUGH == dgraw){
        if(audec->format == ACODEC_FMT_AC3 || audec->format == ACODEC_FMT_EAC3){
            amsysfs_set_sysfs_int("/sys/class/audiodsp/digital_codec",2);
            audec->codec_type=1;
        }else if(audec->format == ACODEC_FMT_DTS){
            amsysfs_set_sysfs_int("/sys/class/audiodsp/digital_codec",3);
            audec->codec_type=3;

        }
    }else if(AUDIO_HDMI_PASSTHROUGH == dgraw){
        if(audec->format == ACODEC_FMT_AC3){
            amsysfs_set_sysfs_int("/sys/class/audiodsp/digital_codec",2);
            audec->codec_type=1;
        }else if(audec->format == ACODEC_FMT_EAC3){
            amsysfs_set_sysfs_int("/sys/class/audiodsp/digital_codec",4);
            audec->codec_type=2;
        }else if(audec->format == ACODEC_FMT_DTS){
            amsysfs_set_sysfs_int("/sys/class/audiodsp/digital_codec",3);
            audec->codec_type=3;

        }

    }

    if(AUDIO_PCM_OUTPUT == dgraw) {
        adec_print("OUT SETTING::PCM\n");
        out_ops->private_data_raw = NULL;
        return -1;
    }


    alsa_param = (alsa_param_t *)malloc(sizeof(alsa_param_t));
    if (!alsa_param) {
        adec_print("alloc alsa_param failed, not enough memory!");
        return -1;
    }
    memset(alsa_param, 0, sizeof(alsa_param_t));
    
    if (audec->samplerate >= (88200 + 96000) / 2) {
        alsa_param->flag = 1;
        alsa_param->oversample = -1;
        alsa_param->rate = 48000;
    } else if (audec->samplerate >= (64000 + 88200) / 2) {
        alsa_param->flag = 1;
        alsa_param->oversample = -1;
        alsa_param->rate = 44100;
    } else if (audec->samplerate >= (48000 + 64000) / 2) {
        alsa_param->flag = 1;
        alsa_param->oversample = -1;
        alsa_param->rate = 32000;
    } else if (audec->samplerate >= (44100 + 48000) / 2) {
        alsa_param->oversample = 0;
        alsa_param->rate = 48000;
        if (audec->channels == 1) {
            alsa_param->flag = 1;
        } else if (audec->channels == 2) {
            alsa_param->flag = 0;
        }
    } else if (audec->samplerate >= (32000 + 44100) / 2) {
        alsa_param->oversample = 0;
        alsa_param->rate = 44100;
        if (audec->channels == 1) {
            alsa_param->flag = 1;
        } else if (audec->channels == 2) {
            alsa_param->flag = 0;
        }
    } else if (audec->samplerate >= (24000 + 32000) / 2) {
        alsa_param->oversample = 0;
        alsa_param->rate = 32000;
        if (audec->channels == 1) {
            alsa_param->flag = 1;
        } else if (audec->channels == 2) {
            alsa_param->flag = 0;
        }
    } else if (audec->samplerate >= (22050 + 24000) / 2) {
        alsa_param->flag = 1;
        alsa_param->oversample = 1;
        alsa_param->rate = 48000;
    } else if (audec->samplerate >= (16000 + 22050) / 2) {
        alsa_param->flag = 1;
        alsa_param->oversample = 1;
        alsa_param->rate = 44100;
    } else if (audec->samplerate >= (12000 + 16000) / 2) {
        alsa_param->flag = 1;
        alsa_param->oversample = 1;
        alsa_param->rate = 32000;
    } else if (audec->samplerate >= (11025 + 12000) / 2) {
        alsa_param->flag = 1;
        alsa_param->oversample = 2;
        alsa_param->rate = 48000;
    } else if (audec->samplerate >= (8000 + 11025) / 2) {
        alsa_param->flag = 1;
        alsa_param->oversample = 2;
        alsa_param->rate = 44100;
    } else {
        alsa_param->flag = 1;
        alsa_param->oversample = 2;
        alsa_param->rate = 32000;
    }

    alsa_param->channelcount = 2;
    alsa_param->realchanl = audec->channels;
    //alsa_param->rate = audec->samplerate;
    alsa_param->format = SND_PCM_FORMAT_S16_LE;
    alsa_param->wait_flag=0;

    sound_card_id = alsa_get_aml_card();
    if (sound_card_id  < 0) {
        sound_card_id = 0;
        adec_print("[%s::%d]--[get aml card fail, use default]\n",__FUNCTION__, __LINE__);
    }
    adec_print("[%s::%d]--[alsa_get_aml_card return:%d]\n",__FUNCTION__, __LINE__,sound_card_id);

    sound_dev_id = alsa_get_spdif_port();
    if (sound_dev_id < 0) {
        sound_dev_id = 0;
        adec_print("[%s::%d]--[get aml card device fail, use default]\n",__FUNCTION__, __LINE__);
    }

    adec_print("[%s::%d]--[sound_dev_id(spdif) return:%d]\n",__FUNCTION__, __LINE__,sound_dev_id);

    sprintf(sound_card_dev, "hw:%d,%d", sound_card_id, sound_dev_id);

    adec_print("[%s::%d]--[sound_card_dev:%s]\n",__FUNCTION__, __LINE__,sound_card_dev);

    err = snd_pcm_open(&alsa_param->handle, sound_card_dev, SND_PCM_STREAM_PLAYBACK, 0);
    if (err < 0) {
        adec_print("[%s::%d]--[audio open error: %s]\n", __FUNCTION__, __LINE__,snd_strerror(err));

        snd_pcm_drop(alsa_param->handle);
        snd_pcm_close(alsa_param->handle);
        return -1;
    } else {
        adec_print("[%s::%d]--[audio open(snd_pcm_open) successfully]\n", __FUNCTION__, __LINE__);
    }


    readi_func = snd_pcm_readi;
    writei_func = snd_pcm_writei;
    readn_func = snd_pcm_readn;
    writen_func = snd_pcm_writen;

    set_params_raw(alsa_param);

    out_ops->private_data_raw = (void *)alsa_param;

    /*TODO:  create play thread */
    pthread_mutex_init(&alsa_param->playback_mutex, NULL);
    pthread_cond_init(&alsa_param->playback_cond, NULL);
    err = pthread_create(&tid, NULL, (void *)alsa_playback_raw_loop, (void *)audec);
    if (err != 0) {
        adec_print("alsa_playback_raw_loop thread create failed!");
        snd_pcm_close(alsa_param->handle);
        return -1;
    }
    adec_print("Create alsa_playback_raw_loop thread success ! tid = %d\n", tid);

    alsa_param->playback_tid = tid;

    alsactl_parser();
    return 0;
}

/**
 * \brief start output
 * \param audec pointer to audec
 * \return 0 on success otherwise negative error code
 *
 * Call alsa_start(), then the callback will start being called.
 */
int alsa_start_raw(struct aml_audio_dec* audec)
{
    alsa_param_t *alsa_param;

    audio_out_operations_t *out_ops = &audec->aout_ops;
    if(out_ops->private_data_raw) {
        alsa_param = (alsa_param_t *)out_ops->private_data_raw;
    } else {
        adec_print("OUT SETTING::PCM\n");
        return -1;
    }

    pthread_mutex_lock(&alsa_param->playback_mutex);
    adec_print("yvonne pthread_cond_signalalsa_param->wait_flag=1\n");
    alsa_param->wait_flag=1;//yvonneadded
    pthread_cond_signal(&alsa_param->playback_cond);
    pthread_mutex_unlock(&alsa_param->playback_mutex);

    return 0;
}

/**
 * \brief pause output
 * \param audec pointer to audec
 * \return 0 on success otherwise negative error code
 */
int alsa_pause_raw(struct aml_audio_dec* audec)
{

    int res = 0;
    alsa_param_t *alsa_params;

    if(audec->aout_ops.private_data_raw) {
        alsa_params = (alsa_param_t *)audec->aout_ops.private_data_raw;
    } else {
        adec_print("OUT SETTING::PCM\n");
        return -1;
    }
    if(1 == alsa_params->pause_flag) {
        adec_print("[%s::%d]--[already in pause(%d) status]\n",__FUNCTION__, __LINE__,alsa_params->pause_flag);
        return 0;
    }

    alsa_params->pause_flag = 1;
    while ((res = snd_pcm_pause(alsa_params->handle, 1)) == -EAGAIN) {
        sleep(1);
    }

    return res;
}

/**
 * \brief resume output
 * \param audec pointer to audec
 * \return 0 on success otherwise negative error code
 */
int alsa_resume_raw(struct aml_audio_dec* audec)
{

    int res = 0;
    alsa_param_t *alsa_params;

    if(audec->aout_ops.private_data_raw) {
        alsa_params = (alsa_param_t *)audec->aout_ops.private_data_raw;
    } else {
        adec_print("OUT SETTING::PCM\n");
        return -1;
    }

    alsa_params->pause_flag = 0;
    while ((res = snd_pcm_pause(alsa_params->handle, 0)) == -EAGAIN) {
        sleep(1);
    }

    return res;
}

/**
 * \brief stop output
 * \param audec pointer to audec
 * \return 0 on success otherwise negative error code
 */
int alsa_stop_raw(struct aml_audio_dec* audec)
{
    alsa_param_t *alsa_params;
    int res = 0;
    int dgraw = amsysfs_get_sysfs_int("/sys/class/audiodsp/digital_raw");

    if(audec->aout_ops.private_data_raw) {
        alsa_params = (alsa_param_t *)audec->aout_ops.private_data_raw;
    } else {
        adec_print("OUT SETTING::PCM\n");
        return -1;
    }

    //resume the spdif card, otherwise the output will block
    if(alsa_params->pause_flag == 1){
        while ((res = snd_pcm_pause(alsa_params->handle, 0)) == -EAGAIN) {
            usleep(1000);
        }
    }

    alsa_params->pause_flag = 0; //we should clear pause flag ,as we can stop from paused state
    alsa_params->stop_flag = 1;
    alsa_params->wait_flag = 0;

    pthread_cond_signal(&alsa_params->playback_cond);
    pthread_join(alsa_params->playback_tid, NULL);
    pthread_mutex_destroy(&alsa_params->playback_mutex);
    pthread_cond_destroy(&alsa_params->playback_cond);


    snd_pcm_drop(alsa_params->handle);
    snd_pcm_close(alsa_params->handle);

    free(alsa_params);
    audec->aout_ops.private_data_raw = NULL;
    adec_print("exit alsa out raw stop\n");
    if((AUDIO_SPDIF_PASSTHROUGH == dgraw)||(AUDIO_HDMI_PASSTHROUGH == dgraw)){
        if((audec->format == ACODEC_FMT_AC3) ||(audec->format == ACODEC_FMT_EAC3) || \
			(audec->format == ACODEC_FMT_DTS)){
            amsysfs_set_sysfs_int("/sys/class/audiodsp/digital_codec",0);
        }
    }

    return 0;
}



/**
 * \brief get output latency in ms
 * \param audec pointer to audec
 * \return output latency
 */
unsigned long alsa_latency_raw(struct aml_audio_dec* audec)
{
    return 0;
}

/**
* alsa dumy codec controls interface
*/
int dummy_alsa_control_raw(char * id_string , long vol, int rw, long * value){
    int err;
    snd_hctl_t *hctl;
    snd_ctl_elem_id_t *id;
    snd_hctl_elem_t *elem;
    snd_ctl_elem_value_t *control;
    snd_ctl_elem_info_t *info;
    snd_ctl_elem_type_t type;
    unsigned int idx = 0, count;
    long tmp, min, max;
    char dev[10] = {0};
    int card = alsa_get_aml_card();
    int port = alsa_get_spdif_port();

    adec_print("card = %d, port = %d\n", card, port);
    sprintf(dev, "hw:%d,%d", (card >= 0) ? card : 0, (port >= 0) ? port : 0);

    if ((err = snd_hctl_open(&hctl, dev, 0)) < 0) { 
        printf("Control %s open error: %s\n", dev, snd_strerror(err));
        return err;
    }
    if (err = snd_hctl_load(hctl)< 0) {
        printf("Control %s open error: %s\n", dev, snd_strerror(err));
        return err;
    }
    snd_ctl_elem_id_alloca(&id);
    snd_ctl_elem_id_set_interface(id, SND_CTL_ELEM_IFACE_MIXER);
    snd_ctl_elem_id_set_name(id, id_string);
    elem = snd_hctl_find_elem(hctl, id);
    snd_ctl_elem_value_alloca(&control);
    snd_ctl_elem_value_set_id(control, id);
    snd_ctl_elem_info_alloca(&info);
    if ((err = snd_hctl_elem_info(elem, info)) < 0) {
        printf("Control %s snd_hctl_elem_info error: %s\n", dev, snd_strerror(err));
        return err;
    }    
    count = snd_ctl_elem_info_get_count(info);
    type = snd_ctl_elem_info_get_type(info);
    
    for (idx = 0; idx < count; idx++) {
        switch (type) {
            case SND_CTL_ELEM_TYPE_BOOLEAN:
                if(rw){
                    tmp = 0;
                    if (vol >= 1) {
                        tmp = 1;
                    }
                    snd_ctl_elem_value_set_boolean(control, idx, tmp);
                    err = snd_hctl_elem_write(elem, control);
                }else             	
            *value = snd_ctl_elem_value_get_boolean(control, idx);
            break;
            case SND_CTL_ELEM_TYPE_INTEGER:
                if(rw){
            min = snd_ctl_elem_info_get_min(info);
            max = snd_ctl_elem_info_get_max(info);
                    if ((vol >= min) && (vol <= max))
                        tmp = vol;
            else if (vol < min)
                        tmp = min;
                    else if (vol > max)
                        tmp = max;
                    snd_ctl_elem_value_set_integer(control, idx, tmp);
                    err = snd_hctl_elem_write(elem, control);
                }else             	
                *value = snd_ctl_elem_value_get_integer(control, idx);
            break;
            default:
                printf("?");
            break;
        }
        if (err < 0){
            printf ("control%s access error=%s,close control device\n", dev, snd_strerror(err));
            snd_hctl_close(hctl);
            return err;
        }
    }
    
    return 0;
}
/**
 * \brief mute output
 * \param audec pointer to audec
 * \param en  1 = mute, 0 = unmute
 * \return 0 on success otherwise negative error code
 */
int alsa_mute_raw(struct aml_audio_dec* audec, adec_bool_t en){

    if(audec->aout_ops.private_data_raw) {
        alsa_param_t *alsa_params = (alsa_param_t *)audec->aout_ops.private_data_raw;
    } else {
        adec_print("OUT SETTING::PCM\n");
        return -1;
    }

    return 0;
}

#if 0
/**
 * \brief get output handle
 * \param audec pointer to audec
 */
void get_output_func(struct aml_audio_dec* audec)
{
    audio_out_operations_t *out_ops = &audec->aout_ops;

    out_ops->init = alsa_init_raw;
    out_ops->start = alsa_start_raw;
    out_ops->pause = alsa_pause_raw;
    out_ops->resume = alsa_resume_raw;
    out_ops->stop = alsa_stop_raw;
    out_ops->latency = alsa_latency_raw;
    out_ops->mute = alsa_mute_raw;
}
#endif

