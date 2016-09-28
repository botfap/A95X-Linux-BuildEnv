/**
 * \file log-print.h
 * \brief  Definitiond Of Print Functions
 * \version 1.0.0
 * \date 2011-03-08
 */
/* Copyright (C) 2007-2011, Amlogic Inc.
 * All right reserved
 *
 */
#ifndef AM_LOG_H
#define AM_LOG_H
#include <stdio.h>
#ifndef LOGD
#ifdef ANDROID
#include <utils/Log.h>
    #define LOGV ALOGV
    #define LOGD ALOGD
    #define LOGI ALOGI
    #define LOGW ALOGW
    #define LOGE ALOGE
#else
    #define LOGV printf
    #define LOGD printf
    #define LOGI printf
    #define LOGW printf
    #define LOGE printf
#endif
#endif

#endif
