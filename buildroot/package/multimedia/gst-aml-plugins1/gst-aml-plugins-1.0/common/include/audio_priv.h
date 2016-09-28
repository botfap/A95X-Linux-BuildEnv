/**
* @file audio_priv.h
* @brief  Funtion prototypes of audio lib
* @author Zhang Chen <chen.zhang@amlogic.com>
* @version 1.0.0
* @date 2011-02-24
*/
/* Copyright (C) 2007-2011, Amlogic Inc.
* All right reserved
* 
*/
#ifndef CODEC_PRIV_H_
#define CODEC_PRIV_H_
void audio_start(void **priv, codec_para_t *pcodec);
void audio_stop(void **priv);
void audio_pause(void *priv);
void audio_resume(void *priv);
#endif
