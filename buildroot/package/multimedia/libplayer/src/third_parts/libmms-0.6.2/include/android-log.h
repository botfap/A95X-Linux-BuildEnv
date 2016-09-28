/*
** AACPlayer - Freeware Advanced Audio (AAC) Player for Android
** Copyright (C) 2011 Spolecne s.r.o., http://www.spoledge.com
**  
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
** (at your option) any later version.
** 
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
** 
** You should have received a copy of the GNU General Public License
** along with this program. If not, see <http://www.gnu.org/licenses/>.
**/

#ifndef ANDROID_LOG_H
#define ANDROID_LOG_H

#include <android/log.h>


#ifdef AACD_LOGLEVEL_VERBOSE
#define ALOG_VERBOSE(...) \
    __android_log_print(ANDROID_LOG_VERBOSE, ANDROID_LOG_MODULE, __VA_ARGS__)
#else
#define ALOG_VERBOSE(...) //
#endif

#ifdef AACD_LOGLEVEL_DEBUG
#define ALOG_DEBUG(...) \
    __android_log_print(ANDROID_LOG_DEBUG, ANDROID_LOG_MODULE, __VA_ARGS__)
#else
#define ALOG_DEBUG(...) //
#endif

#ifdef AACD_LOGLEVEL_INFO
#define AACD_INFO(...) \
    __android_log_print(ANDROID_LOG_INFO, ANDROID_LOG_MODULE, __VA_ARGS__)
#else
#define ALOG_INFO(...) //
#endif

#ifdef ALOG_LOGLEVEL_WARN
#define ALOG_WARN(...) \
    __android_log_print(ANDROID_LOG_WARN, ANDROID_LOG_MODULE, __VA_ARGS__)
#else
#define ALOG_WARN(...) //
#endif

#ifdef AACD_LOGLEVEL_ERROR
#define ALOG_ERROR(...) \
    __android_log_print(ANDROID_LOG_ERROR, ANDROID_LOG_MODULE, __VA_ARGS__)
#else
#error "AACD_LOGLEVEL_ERROR is not defined"
#define ALOG_ERROR(...) //
#endif

#endif  // ANDROID_LOG_H
