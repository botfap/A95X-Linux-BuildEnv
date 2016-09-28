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

static int reset_track_enable=0;
void adec_reset_track_enable(int enable_flag)
{
    reset_track_enable=enable_flag;
	adec_print("reset_track_enable=%d\n", reset_track_enable);
}

static int get_sysfs_int(const char *path)
{
    return amsysfs_get_sysfs_int(path);
}

static int set_sysfs_int(const char *path, int val)
{
    return amsysfs_set_sysfs_int(path, val);
}

static int audiodsp_get_format_changed_flag()
{
    return get_sysfs_int("/sys/class/audiodsp/format_change_flag");

}

 void audiodsp_set_format_changed_flag( int val)
{
    set_sysfs_int("/sys/class/audiodsp/format_change_flag", val);

}

static int audiodsp_get_pcm_resample_enable()
{
    int utils_fd, ret;
    unsigned long value;

    utils_fd = open("/dev/amaudio_utils", O_RDWR);
    if (utils_fd >= 0) {
        ret = ioctl(utils_fd, AMAUDIO_IOC_GET_RESAMPLE_ENA, &value);
        if (ret < 0) {
            adec_print("AMAUDIO_IOC_GET_RESAMPLE_ENA failed\n");
            close(utils_fd);
            return -1;
        }
        close(utils_fd);
        return value;
    }
    return -1;
}

static int audiodsp_set_pcm_resample_enable(unsigned long enable)
{
    int utils_fd, ret;

    utils_fd = open("/dev/amaudio_utils", O_RDWR);
    if (utils_fd >= 0) {
        ret = ioctl(utils_fd, AMAUDIO_IOC_SET_RESAMPLE_ENA, enable);
        if (ret < 0) {
            adec_print("AMAUDIO_IOC_SET_RESAMPLE_ENA failed\n");
            close(utils_fd);
            return -1;
        }
        close(utils_fd);
        return 0;
    }
    return -1;
}

void adec_reset_track(aml_audio_dec_t *audec)
{
    if(audec->format_changed_flag && audec->state >= INITTED){
        adec_print("reset audio_track: samplerate=%d channels=%d\n", audec->samplerate,audec->channels);
        audio_out_operations_t *out_ops = &audec->aout_ops;
        out_ops->mute(audec, 1);
		//out_ops->pause(audec);//otherwise will block indefinitely at the writei_func in func pcm_write
        out_ops->stop(audec);
        //audec->SessionID +=1;
        out_ops->init(audec);
        if(audec->state == ACTIVE)
        	out_ops->start(audec);
        audec->format_changed_flag=0;
    }
}

int audiodsp_format_update(aml_audio_dec_t *audec)
{
    int m_fmt;
    int ret = -1;
    unsigned long val;
    dsp_operations_t *dsp_ops = &audec->adsp_ops;
	
    if (dsp_ops->dsp_file_fd < 0 || get_audio_decoder()!=AUDIO_ARC_DECODER) {
        return ret;
    }
	
    ret=0;
    if(1/*audiodsp_get_format_changed_flag()*/)
    {
        ioctl(dsp_ops->dsp_file_fd, AUDIODSP_GET_CHANNELS_NUM, &val);
        if (val != (unsigned long) - 1) {
            if( audec->channels != val){
                //adec_print("dsp_format_update: pre_channels=%d  cur_channels=%d\n", audec->channels,val);
                audec->channels = val;
                ret=1;
            }
        }

         ioctl(dsp_ops->dsp_file_fd, AUDIODSP_GET_SAMPLERATE, &val);
         if (val != (unsigned long) - 1) {
            if(audec->samplerate != val){
                //adec_print("dsp_format_update: pre_samplerate=%d  cur_samplerate=%d\n", audec->samplerate,val);
                audec->samplerate = val;
                ret=2;
            }
         }
         #if 1
         ioctl(dsp_ops->dsp_file_fd, AUDIODSP_GET_BITS_PER_SAMPLE, &val);
         if (val != (unsigned long) - 1) {
            if(audec->data_width != val){
                //adec_print("dsp_format_update: pre_data_width=%d  cur_data_width=%d\n", audec->data_width,val);
                audec->data_width = val;
                ret=3;
            }
        }
        #endif
		//audiodsp_set_format_changed_flag(0);        
        if (am_getconfig_bool("media.libplayer.wfd")) {
	    ret = ioctl(dsp_ops->dsp_file_fd, AUDIODSP_GET_PCM_LEVEL, &val);
            if (ret == 0) {
                //adec_print("pcm level == 0x%x\n", val);
                if ((val < 0x1000) && (1==audiodsp_get_pcm_resample_enable())) {
                //    adec_print("disable pcm down resample");
                //    audiodsp_set_pcm_resample_enable(0);
                }
            } 
        }
    }
    if(ret>0){
        audec->format_changed_flag=ret;
        adec_print("dsp_format_update: audec->format_changed_flag = %d \n", audec->format_changed_flag); 
    }
    return ret;
}



int audiodsp_get_pcm_left_len()
{
    return get_sysfs_int("/sys/class/audiodsp/pcm_left_len");

}




