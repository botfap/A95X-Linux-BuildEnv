/**
* @file codec_h_ctrl.h
* @brief  Definition of codec devices and function prototypes
* @author Zhang Chen <chen.zhang@amlogic.com>
* @version 1.0.0
* @date 2011-02-24
*/
/* Copyright (C) 2007-2011, Amlogic Inc.
* All right reserved
* 
*/

#ifndef CODEC_HEADER_H_H
#define CODEC_HEADER_H_H
#include <codec_type.h>
#include <codec_error.h>

#define CODEC_DEBUG

#ifdef CODEC_DEBUG
#ifdef ANDROID
#include <android/log.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#define  LOG_TAG    "amcodec"
#define CODEC_PRINT(...) __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#else
#define CODEC_PRINT(f,s...) fprintf(stderr,f,##s)
#endif
#else
#define CODEC_PRINT(f,s...)
#endif

#define CODEC_VIDEO_ES_DEVICE       "/dev/amstream_vbuf"
#define CODEC_AUDIO_ES_DEVICE       "/dev/amstream_abuf"
#define CODEC_TS_DEVICE             "/dev/amstream_mpts"
#define CODEC_PS_DEVICE             "/dev/amstream_mpps"
#define CODEC_RM_DEVICE             "/dev/amstream_rm"
#define CODEC_CNTL_DEVICE           "/dev/amvideo"
#define CODEC_SUB_DEVICE            "/dev/amstream_sub"
#define CODEC_SUB_READ_DEVICE       "/dev/amstream_sub_read"
#define CODEC_AUDIO_UTILS_DEVICE    "/dev/amaudio_utils"
#define CODEC_VIDEO_ES_HEVC_DEVICE  "/dev/amstream_hevc"

CODEC_HANDLE codec_h_open(const char *port_addr, int flags);
int codec_h_close(CODEC_HANDLE h);
int codec_h_write(CODEC_HANDLE , void *, int);
int codec_h_read(CODEC_HANDLE, void *, int);
int codec_h_control(CODEC_HANDLE h, int cmd, unsigned long paramter);
void codec_h_set_support_new_cmd(int value);
int codec_h_is_support_new_cmd();


#endif
