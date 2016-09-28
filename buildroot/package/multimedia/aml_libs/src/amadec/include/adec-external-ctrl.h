/**
 * \file adec-external-ctrl.h
 * \brief  Function prototypes of Audio Dec
 * \version 1.0.0
 * \date 2011-03-08
 */
/* Copyright (C) 2007-2011, Amlogic Inc.
 * All right reserved
 *
 */
#ifndef ADEC_EXTERNAL_H
#define ADEC_EXTERNAL_H

#ifdef  __cplusplus
extern "C"
{
#endif

    int audio_decode_init(void **handle, arm_audio_info *pcodec);
    int audio_decode_start(void *handle);
    int audio_decode_pause(void *handle);
    int audio_decode_resume(void *handle);
    int audio_decode_stop(void *handle);
    int audio_decode_release(void **handle);
    int audio_decode_automute(void *, int);
    int audio_decode_set_mute(void *handle, int);
    int audio_decode_set_volume(void *, float);
    int audio_decode_get_volume(void *, float *);
    int audio_channels_swap(void *);
    int audio_channel_left_mono(void *);
    int audio_channel_right_mono(void *);
    int audio_channel_stereo(void *);
    int audio_output_muted(void *handle);
    int audio_dec_ready(void *handle);
    int audio_get_decoded_nb_frames(void *handle);

    int audio_decode_set_lrvolume(void *, float lvol,float rvol);	
    int audio_decode_get_lrvolume(void *, float* lvol,float* rvol);	
    int audio_set_av_sync_threshold(void *, int);
    int audio_get_soundtrack(void *, int* );	
	int get_audio_decoder(void);
	int get_decoder_status(void *p,struct adec_status *adec);
    int dummy_decode_set_mute(int en);
    int dummy_decode_set_volume(int vol);
    int dummy_decode_get_volume(int *vol);

#ifdef  __cplusplus
}
#endif

#endif
