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
#define USE_INTERPOLATION

static snd_pcm_sframes_t (*readi_func)(snd_pcm_t *handle, void *buffer, snd_pcm_uframes_t size);
static snd_pcm_sframes_t (*writei_func)(snd_pcm_t *handle, const void *buffer, snd_pcm_uframes_t size);
static snd_pcm_sframes_t (*readn_func)(snd_pcm_t *handle, void **bufs, snd_pcm_uframes_t size);
static snd_pcm_sframes_t (*writen_func)(snd_pcm_t *handle, void **bufs, snd_pcm_uframes_t size);


static int fragcount = PERIOD_NUM;
static snd_pcm_uframes_t chunk_size = PERIOD_SIZE;
static char output_buffer[64 * 1024];
static unsigned char decode_buffer[OUTPUT_BUFFER_SIZE + 64];

static char sound_card_dev[10] = {0};



#ifdef USE_INTERPOLATION
static int pass1_history[8][8];
#pragma align_to(64,pass1_history)
static int pass2_history[8][8];
#pragma align_to(64,pass2_history)
static short pass1_interpolation_output[0x4000];
#pragma align_to(64,pass1_interpolation_output)
static short interpolation_output[0x8000];
#pragma align_to(64,interpolation_output)

static inline short CLIPTOSHORT(int x)
{
    short res;
#if 0
    __asm__ __volatile__(
        "min  r0, %1, 0x7fff\r\n"
        "max r0, r0, -0x8000\r\n"
        "mov %0, r0\r\n"
        :"=r"(res)
        :"r"(x)
        :"r0"
    );
#else
    if (x > 0x7fff) {
        res = 0x7fff;
    } else if (x < -0x8000) {
        res = -0x8000;
    } else {
        res = x;
    }
#endif
    return res;
}

static void pcm_interpolation(int interpolation, unsigned num_channel, unsigned num_sample, short *samples)
{
    int i, k, l, ch;
    int *s;
    short *d;
    for (ch = 0; ch < num_channel; ch++) {
        s = pass1_history[ch];
        if (interpolation < 2) {
            d = interpolation_output;
        } else {
            d = pass1_interpolation_output;
        }
        for (i = 0, k = l = ch; i < num_sample; i++, k += num_channel) {
            s[0] = s[1];
            s[1] = s[2];
            s[2] = s[3];
            s[3] = s[4];
            s[4] = s[5];
            s[5] = samples[k];
            d[l] = s[2];
            l += num_channel;
            d[l] = CLIPTOSHORT((150 * (s[2] + s[3]) - 25 * (s[1] + s[4]) + 3 * (s[0] + s[5]) + 128) >> 8);
            l += num_channel;
        }
        if (interpolation >= 2) {
            s = pass2_history[ch];
            d = interpolation_output;
            for (i = 0, k = l = ch; i < num_sample * 2; i++, k += num_channel) {
                s[0] = s[1];
                s[1] = s[2];
                s[2] = s[3];
                s[3] = s[4];
                s[4] = s[5];
                s[5] = pass1_interpolation_output[k];
                d[l] = s[2];
                l += num_channel;
                d[l] = CLIPTOSHORT((150 * (s[2] + s[3]) - 25 * (s[1] + s[4]) + 3 * (s[0] + s[5]) + 128) >> 8);
                l += num_channel;
            }
        }
    }
}
#endif


static int set_params(alsa_param_t *alsa_params)
{
    snd_pcm_hw_params_t *hwparams;
    snd_pcm_sw_params_t *swparams;
    //  snd_pcm_uframes_t buffer_size;
    //  snd_pcm_uframes_t boundary;
    //  unsigned int period_time = 0;
    //  unsigned int buffer_time = 0;
    snd_pcm_uframes_t bufsize;
    int err = 0;
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

    bufsize = 	PERIOD_NUM*PERIOD_SIZE;
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
    printf("alsa buffer frame size %d \n",bufsize);
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

static size_t pcm_write(alsa_param_t * alsa_param, u_char * data, size_t count)
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

static unsigned oversample_play(alsa_param_t * alsa_param, char * src, unsigned count)
{
    int frames = 0;
    int ret, i;
    unsigned short * to, *from;
    to = (unsigned short *)output_buffer;
    from = (unsigned short *)src;

    if (alsa_param->realchanl == 2) {
        if (alsa_param->oversample == -1) {
            frames = count * 8 / alsa_param->bits_per_frame;
            frames = frames & (~(32 - 1));
            for (i = 0; i < (frames * 2); i += 4) { // i for sample
                *to++ = *from++;
                *to ++ = *from++;
                from += 2;
            }
            ret = pcm_write(alsa_param, output_buffer, frames / 2);
            ret = ret * alsa_param->bits_per_frame / 8;
            ret = ret * 2;
        } else if (alsa_param->oversample == 1) {
            frames = count * 8 / alsa_param->bits_per_frame;
            frames = frames & (~(16 - 1));
#ifdef USE_INTERPOLATION
            pcm_interpolation(1, alsa_param->realchanl, frames, (short*)src);
            memcpy(output_buffer, interpolation_output, (frames * alsa_param->bits_per_frame / 4));
#else
            short l, r;
            for (i = 0; i < (frames * 2); i += 2) {
                l = *from++;
                r = *from++;
                *to++ = l;
                *to++ = r;
                *to++ = l;
                *to++ = r;
            }
#endif
            ret = pcm_write(alsa_param, output_buffer, frames * 2);
            ret = ret * alsa_param->bits_per_frame / 8;
            ret = ret / 2;
        } else if (alsa_param->oversample == 2) {
            frames = count * 8 / alsa_param->bits_per_frame;
            frames = frames & (~(8 - 1));
#ifdef USE_INTERPOLATION
            pcm_interpolation(2, alsa_param->realchanl, frames, (short*)src);
            memcpy(output_buffer, interpolation_output, (frames * alsa_param->bits_per_frame / 2));
#else
            short l, r;
            for (i = 0; i < (frames * 2); i += 2) {
                l = *from++;
                r = *from++;
                *to++ = l;
                *to++ = r;
                *to++ = l;
                *to++ = r;
                *to++ = l;
                *to++ = r;
                *to++ = l;
                *to++ = r;
            }
#endif
            ret = pcm_write(alsa_param, output_buffer, frames * 4);
            ret = ret * alsa_param->bits_per_frame / 8;
            ret = ret / 4;
        }
    } else if (alsa_param->realchanl == 1) {
        if (alsa_param->oversample == -1) {
            frames = count * 8 / alsa_param->bits_per_frame;
            frames = frames & (~(32 - 1));
            for (i = 0; i < (frames * 2); i += 2) {
                *to++ = *from;
                *to++ = *from++;
                from++;
            }
            ret = pcm_write(alsa_param, output_buffer, frames);
            ret = ret * alsa_param->bits_per_frame / 8;
        } else if (alsa_param->oversample == 0) {
            frames = count * 8 / (alsa_param->bits_per_frame >> 1);
            frames = frames & (~(16 - 1));
            for (i = 0; i < (frames); i++) {
                *to++ = *from;
                *to++ = *from++;
            }
            ret = pcm_write(alsa_param, output_buffer, frames);
            ret = ret * (alsa_param->bits_per_frame) / 8;
            ret = ret / 2;
        } else if (alsa_param->oversample == 1) {
            frames = count * 8 / (alsa_param->bits_per_frame >> 1);
            frames = frames & (~(8 - 1));
#ifdef USE_INTERPOLATION
            pcm_interpolation(1, alsa_param->realchanl, frames, (short*)src);
            from = (unsigned short*)interpolation_output;
            for (i = 0; i < (frames * 2); i++) {
                *to++ = *from;
                *to++ = *from++;
            }
#else
            for (i = 0; i < (frames); i++) {
                *to++ = *from;
                *to++ = *from;
                *to++ = *from;
                *to++ = *from++;
            }
#endif
            ret = pcm_write(alsa_param, output_buffer, frames * 2);
            ret = ret * (alsa_param->bits_per_frame) / 8;
            ret = ret / 4;
        } else if (alsa_param->oversample == 2) {
            frames = count * 8 / (alsa_param->bits_per_frame >> 1);
            frames = frames & (~(8 - 1));
#ifdef USE_INTERPOLATION
            pcm_interpolation(2, alsa_param->realchanl, frames, (short*)src);
            from = (unsigned short*)interpolation_output;
            for (i = 0; i < (frames * 4); i++) {
                *to++ = *from;
                *to++ = *from++;
            }
#else
            for (i = 0; i < (frames); i++) {
                *to++ = *from;
                *to++ = *from;
                *to++ = *from;
                *to++ = *from;
                *to++ = *from;
                *to++ = *from;
                *to++ = *from;
                *to++ = *from++;
            }
#endif
            ret = pcm_write(alsa_param, output_buffer, frames * 4);
            ret = ret * (alsa_param->bits_per_frame) / 8;
            ret = ret / 8;
        }
    }

    return ret;
}

static int alsa_play(alsa_param_t * alsa_param, char * data, unsigned len)
{
    size_t l = 0, r;

    if (!alsa_param->flag) {
        l = len * 8 / alsa_param->bits_per_frame;
        l = l & (~(32 - 1)); /*driver only support  32 frames each time */
        r = pcm_write(alsa_param, data, l);
        r = r * alsa_param->bits_per_frame / 8;
    } else {
        r = oversample_play(alsa_param, data, len);
    }

    return r ;
}

static void *alsa_playback_loop(void *args)
{
    int len = 0;
    int len2 = 0;
    int offset = 0;
    aml_audio_dec_t *audec;
    alsa_param_t *alsa_params;
    unsigned char *buffer = (unsigned char *)(((unsigned long)decode_buffer + 32) & (~0x1f));

    audec = (aml_audio_dec_t *)args;
    alsa_params = (alsa_param_t *)audec->aout_ops.private_data;

 /*   pthread_mutex_init(&alsa_params->playback_mutex, NULL);
    pthread_cond_init(&alsa_params->playback_cond, NULL);*/

    pthread_mutex_lock(&alsa_params->playback_mutex);
    
    while( !alsa_params->wait_flag && alsa_params->stop_flag == 0)	
    {
        adec_print("pthread_cond_wait\n");
         pthread_cond_wait(&alsa_params->playback_cond, &alsa_params->playback_mutex);
    }
    alsa_params->wait_flag=1;
    pthread_mutex_unlock(&alsa_params->playback_mutex);

    adec_print("alsa playback loop start to run !\n");

    while (!alsa_params->stop_flag) {
        while ((len < (128 * 2)) && (!alsa_params->stop_flag)) {
            if (offset > 0) {
                memcpy(buffer, buffer + offset, len);
            }
            len2 = audec->adsp_ops.dsp_read(&audec->adsp_ops, (buffer + len), (OUTPUT_BUFFER_SIZE - len));
            len = len + len2;
            offset = 0;
        }

        while (alsa_params->pause_flag) {
            usleep(10000);
        }

        adec_refresh_pts(audec);

        len2 = alsa_play(alsa_params, (buffer + offset), len);
        if (len2 >= 0) {
            len -= len2;
            offset += len2;
        } else {
            len = 0;
            offset = 0;
        }
    }

    adec_print("Exit alsa playback loop !\n");
    pthread_exit(NULL);
    return NULL;
}

/**
 * \brief output initialization
 * \param audec pointer to audec
 * \return 0 on success otherwise negative error code
 */
int alsa_init(struct aml_audio_dec* audec)
{
    adec_print("alsa out init");

    int err = 0;
    pthread_t tid;
    alsa_param_t *alsa_param;
    audio_out_operations_t *out_ops = &audec->aout_ops;
    int sound_card_id = 0;
    int sound_dev_id = 0;
    int dgraw = amsysfs_get_sysfs_int("/sys/class/audiodsp/digital_raw");

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

#ifdef USE_INTERPOLATION
    memset(pass1_history, 0, 64 * sizeof(int));
    memset(pass2_history, 0, 64 * sizeof(int));
#endif

    if(((AUDIO_SPDIF_PASSTHROUGH == dgraw)||(AUDIO_HDMI_PASSTHROUGH == dgraw))&& \
        ((ACODEC_FMT_AC3 == audec->format) || (ACODEC_FMT_EAC3 == audec->format))){

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

        adec_print("[%s::%d]--[sound_dev_id(spdif) return :%d]\n",__FUNCTION__, __LINE__,sound_dev_id);

        if(0 == sound_dev_id) {
            sound_dev_id = 1;
        } else if(1 == sound_dev_id){
            sound_dev_id = 0;
        }
        adec_print("[%s::%d]--[sound_dev_id convert to(pcm):%d]\n",__FUNCTION__, __LINE__,sound_dev_id);

        sprintf(sound_card_dev, "hw:%d,%d", sound_card_id, sound_dev_id);
    }else {
        memcpy(sound_card_dev, PCM_DEVICE_DEFAULT, sizeof(PCM_DEVICE_DEFAULT));
        amsysfs_set_sysfs_int("/sys/class/audiodsp/digital_codec",0);//set output codec type as pcm
    }
    adec_print("[%s::%d]--[sound_card_dev:%s]\n",__FUNCTION__, __LINE__,sound_card_dev);


    err = snd_pcm_open(&alsa_param->handle, sound_card_dev, SND_PCM_STREAM_PLAYBACK, 0);
    if (err < 0) {
        adec_print("[%s::%d]--[audio open error: %s]\n", __FUNCTION__, __LINE__,snd_strerror(err));
        return -1;
    } else {
        adec_print("[%s::%d]--[audio open(snd_pcm_open) successfully]\n", __FUNCTION__, __LINE__);
    }


    readi_func = snd_pcm_readi;
    writei_func = snd_pcm_writei;
    readn_func = snd_pcm_readn;
    writen_func = snd_pcm_writen;

    set_params(alsa_param);

    out_ops->private_data = (void *)alsa_param;

    /*TODO:  create play thread */
    pthread_mutex_init(&alsa_param->playback_mutex, NULL);
    pthread_cond_init(&alsa_param->playback_cond, NULL);
    err = pthread_create(&tid, NULL, (void *)alsa_playback_loop, (void *)audec);
    if (err != 0) {
        adec_print("alsa_playback_loop thread create failed!");
        snd_pcm_close(alsa_param->handle);
        return -1;
    }
    adec_print("Create alsa playback loop thread success ! tid = %d\n", tid);

    alsa_param->playback_tid = tid;

    alsactl_parser();

    //ac3 and eac3 passthrough
    if(((AUDIO_SPDIF_PASSTHROUGH == dgraw)||(AUDIO_HDMI_PASSTHROUGH == dgraw)) && \
        ((ACODEC_FMT_AC3 == audec->format) || (ACODEC_FMT_EAC3 == audec->format)) ){

        int err;
        err = alsa_init_raw(audec);
        if (err != 0) {
            adec_print("alsa_init_raw return error:%d\n", err);
            snd_pcm_close(alsa_param->handle);
            return -1;
        }

    }

    return 0;
}

/**
 * \brief start output
 * \param audec pointer to audec
 * \return 0 on success otherwise negative error code
 *
 * Call alsa_start(), then the callback will start being called.
 */
int alsa_start(struct aml_audio_dec* audec)
{
    adec_print("alsa out start!\n");

    audio_out_operations_t *out_ops = &audec->aout_ops;
    alsa_param_t *alsa_param = (alsa_param_t *)out_ops->private_data;
    int dgraw = amsysfs_get_sysfs_int("/sys/class/audiodsp/digital_raw");

    pthread_mutex_lock(&alsa_param->playback_mutex);
    adec_print("yvonne pthread_cond_signalalsa_param->wait_flag=1\n");
    alsa_param->wait_flag=1;//yvonneadded
    pthread_cond_signal(&alsa_param->playback_cond);
    pthread_mutex_unlock(&alsa_param->playback_mutex);

    //ac3 and eac3 passthrough
    if(((AUDIO_SPDIF_PASSTHROUGH == dgraw)||(AUDIO_HDMI_PASSTHROUGH == dgraw)) && \
        ((ACODEC_FMT_AC3 == audec->format) || (ACODEC_FMT_EAC3 == audec->format)) ) {
        int err = alsa_start_raw(audec);
        if(err) {
            printf("alsa_start_raw return  error: %d\n", err);
        }
    }

    return 0;
}

/**
 * \brief pause output
 * \param audec pointer to audec
 * \return 0 on success otherwise negative error code
 */
int alsa_pause(struct aml_audio_dec* audec)
{
    adec_print("alsa out pause\n");

    int res = 0;
    alsa_param_t *alsa_params;
    int dgraw = amsysfs_get_sysfs_int("/sys/class/audiodsp/digital_raw");

    alsa_params = (alsa_param_t *)audec->aout_ops.private_data;

    alsa_params->pause_flag = 1;
    while ((res = snd_pcm_pause(alsa_params->handle, 1)) == -EAGAIN) {
        sleep(1);
    }

    //ac3 and eac3 passthrough
    if(((AUDIO_SPDIF_PASSTHROUGH == dgraw)||(AUDIO_HDMI_PASSTHROUGH == dgraw)) && \
        ((ACODEC_FMT_AC3 == audec->format) || (ACODEC_FMT_EAC3 == audec->format)) ) {
        int err = alsa_pause_raw(audec);
        if(err) {
            printf("alsa_pause_raw return  error: %d\n", err);
        }
    }


    return res;
}

/**
 * \brief resume output
 * \param audec pointer to audec
 * \return 0 on success otherwise negative error code
 */
int alsa_resume(struct aml_audio_dec* audec)
{
    adec_print("alsa out rsume\n");

    int res = 0;
    alsa_param_t *alsa_params;

    //ac3 and eac3 passthrough
    int dgraw = amsysfs_get_sysfs_int("/sys/class/audiodsp/digital_raw");
    if(((AUDIO_SPDIF_PASSTHROUGH == dgraw)||(AUDIO_HDMI_PASSTHROUGH == dgraw)) && \
        ((ACODEC_FMT_AC3 == audec->format) || (ACODEC_FMT_EAC3 == audec->format)) ) {
        int err = alsa_resume_raw(audec);
        if(err) {
              printf("alsa_resume_raw return  error: %d\n", err);
        }
    }

    alsa_params = (alsa_param_t *)audec->aout_ops.private_data;

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
int alsa_stop(struct aml_audio_dec* audec)
{
    int dgraw = amsysfs_get_sysfs_int("/sys/class/audiodsp/digital_raw");

    int res = 0;
    alsa_param_t *alsa_params;

    alsa_params = (alsa_param_t *)audec->aout_ops.private_data;
    adec_print("[%s::%d] in--[alsa_params:0x%x]\n",__FUNCTION__, __LINE__,alsa_params);

    //resume the spdif card, otherwise the output will block
    if(alsa_params->pause_flag == 1){
        while ((res = snd_pcm_pause(alsa_params->handle, 0)) == -EAGAIN) {
            usleep(1000);
        }
    }

    //exit alsa_playback_loop
    alsa_params->pause_flag = 0; //we should clear pause flag ,as we can stop from paused state
    alsa_params->stop_flag = 1;
    alsa_params->wait_flag = 0;

    //ac3 and eac3 passthrough
    if(((AUDIO_SPDIF_PASSTHROUGH == dgraw)||(AUDIO_HDMI_PASSTHROUGH == dgraw)) && \
        ((ACODEC_FMT_AC3 == audec->format) || (ACODEC_FMT_EAC3 == audec->format)) ) {

        adec_print("enter alsa_stop_raw step\n");
        int err = alsa_stop_raw(audec);
        if(err) {
            printf("alsa_stop_raw return  error: %d\n", err);
        }
    }

    pthread_cond_signal(&alsa_params->playback_cond);
    pthread_join(alsa_params->playback_tid, NULL);
    pthread_mutex_destroy(&alsa_params->playback_mutex);
    pthread_cond_destroy(&alsa_params->playback_cond);


    snd_pcm_drop(alsa_params->handle);
    snd_pcm_close(alsa_params->handle);

    free(alsa_params);
    audec->aout_ops.private_data = NULL;
    adec_print("exit alsa out stop\n");

    return 0;
}

static int alsa_get_space(alsa_param_t * alsa_param)
{
    snd_pcm_status_t *status;
    int ret = 0;

    snd_pcm_status_alloca(&status);
    if ((ret = snd_pcm_status(alsa_param->handle, status)) < 0) {
        adec_print("Cannot get pcm status \n");
        return 0;
    }

    ret = snd_pcm_status_get_avail(status) * alsa_param->bits_per_sample / 8;
    if (ret > alsa_param->buffer_size) {
        ret = alsa_param->buffer_size;
    }
    return ret;
}

/**
 * \brief get output latency in ms
 * \param audec pointer to audec
 * \return output latency
 */
unsigned long alsa_latency(struct aml_audio_dec* audec)
{
    int buffered_data;
    int sample_num;
    alsa_param_t *alsa_param = (alsa_param_t *)audec->aout_ops.private_data;
    buffered_data = alsa_param->buffer_size - alsa_get_space(alsa_param);
    sample_num = buffered_data / (alsa_param->channelcount * (alsa_param->bits_per_sample / 8)); /*16/2*/
    return (sample_num * (1000 / alsa_param->rate));
}

/**
* alsa dumy codec controls interface
*/
int dummy_alsa_control(char * id_string , long vol, int rw, long * value){
    int err = 0;
    snd_hctl_t *hctl;
    snd_ctl_elem_id_t *id;
    snd_hctl_elem_t *elem;
    snd_ctl_elem_value_t *control;
    snd_ctl_elem_info_t *info;
    snd_ctl_elem_type_t type;
    unsigned int idx = 0, count;
    long tmp, min, max;

    if ((err = snd_hctl_open(&hctl, sound_card_dev, 0)) < 0) { 
        printf("Control %s open error: %s\n", sound_card_dev, snd_strerror(err));
        return err;
    }
    if (err = snd_hctl_load(hctl)< 0) {
        printf("Control %s open error: %s\n", sound_card_dev, snd_strerror(err));
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
        printf("Control %s snd_hctl_elem_info error: %s\n", sound_card_dev, snd_strerror(err));
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
            printf ("control%s access error=%s,close control device\n", sound_card_dev, snd_strerror(err));
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
static int alsa_mute(struct aml_audio_dec* audec, adec_bool_t en){

    int dgraw = amsysfs_get_sysfs_int("/sys/class/audiodsp/digital_raw");

    //ac3 and eac3 passthrough
    if(((AUDIO_SPDIF_PASSTHROUGH == dgraw)||(AUDIO_HDMI_PASSTHROUGH == dgraw)) && \
    ((ACODEC_FMT_AC3 == audec->format) || (ACODEC_FMT_EAC3 == audec->format)) ) {
        int err = alsa_mute_raw(audec,en);
        if(err) {
            adec_print("alsa_mute_raw return  error: %d\n", err);
        }
    }

    return 0;
}
/**
 * \brief get output handle
 * \param audec pointer to audec
 */
void get_output_func(struct aml_audio_dec* audec)
{
    audio_out_operations_t *out_ops = &audec->aout_ops;

    out_ops->init = alsa_init;
    out_ops->start = alsa_start;
    out_ops->pause = alsa_pause;
    out_ops->resume = alsa_resume;
    out_ops->stop = alsa_stop;
    out_ops->latency = alsa_latency;
    out_ops->mute = alsa_mute;
}
