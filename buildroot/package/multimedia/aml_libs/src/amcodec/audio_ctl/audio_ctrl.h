/**
* @file audio_ctrl.h
* @brief  Function prototypes of audio control lib
* @author Zhang Chen <chen.zhang@amlogic.com>
* @version 1.0.0
* @date 2011-02-24
*/
/* Copyright (C) 2007-2011, Amlogic Inc.
* All right reserved
* 
*/

#ifndef AUDIO_CTRL_H
#define AUDIO_CTRL_H
void audio_start(void **priv, codec_para_t *pcodec);
void audio_stop(void **priv);
void audio_pause(void *priv);
void audio_resume(void *priv);
void audio_basic_init(void);
#endif

