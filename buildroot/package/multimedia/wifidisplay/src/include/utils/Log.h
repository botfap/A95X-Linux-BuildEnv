/*
 * Copyright (C) 2005 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//
// C/C++ logging functions.  See the logging documentation for API details.
//
// We'd like these to be available from C code (in case we import some from
// somewhere), so this has a C interface.
//
// The output will be correct when the log file is shared between multiple
// threads and/or multiple processes so long as the operating system
// supports O_APPEND.  These calls have mutex-protected data structures
// and so are NOT reentrant.  Do not use LOG in a signal handler.
//
#ifndef _LIBS_UTILS_LOG_H
#define _LIBS_UTILS_LOG_H

#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

// ---------------------------------------------------------------------

#ifndef LOG_NDEBUG
#define LOG_NDEBUG 0
#endif

#ifndef LOG_TAG
#define LOG_TAG "NULL"
#endif

// ---------------------------------------------------------------------
#ifndef ALOGV
#if LOG_NDEBUG
#define ALOGV(format, ...)   ((void)0)
#else
#define ALOGV(format, ...) \
	printf("ALOGV [%s]: LINE: %d: "format"\n", \
	 	LOG_TAG, __LINE__, ##__VA_ARGS__);
#endif
#endif

#ifndef ALOGD
#if LOG_NDEBUG
#define ALOGD(format, ...)   ((void)0)
#else
#define ALOGD(format, ...) \
	 printf("ALOGD [%s]: LINE: %d: "format"\n", \
	 	LOG_TAG, __LINE__, ##__VA_ARGS__);
#endif
#endif

#ifndef ALOGD_IF
#if LOG_NDEBUG
#define ALOGD_IF(cond, format, ...)   ((void)0)
#else
#define ALOGD_IF(cond, format, ...) \
    if((cond)){                              \
        printf("ALOGD_IF [%s]: LINE: %d: "format"\n", \
	 	LOG_TAG, __LINE__, ##__VA_ARGS__);	\
    }
#endif
#endif

#ifndef ALOGI
#if LOG_NDEBUG
#define ALOGI(format, ...)   ((void)0)
#else
#define ALOGI(format, ...) \
	printf("ALOGI [%s]: LINE: %d: "format"\n", \
	 	LOG_TAG, __LINE__, ##__VA_ARGS__);
#endif
#endif

#ifndef ALOGW
#if LOG_NDEBUG
#define ALOGW(format, ...)   ((void)0)
#else
#define ALOGW(format, ...) \
	printf("ALOGW [%s]: LINE: %d: "format"\n", \
	 	LOG_TAG, __LINE__, ##__VA_ARGS__);
#endif
#endif

#ifndef ALOGW_IF
#if LOG_NDEBUG
#define ALOGW_IF(cond, format, ...)   ((void)0)
#else
#define ALOGW_IF(cond, format, ...) \
    if((cond)){                              \
        printf("ALOGW_IF [%s]: LINE: %d: "format"\n", \
	 	LOG_TAG, __LINE__, ##__VA_ARGS__);	\
    }
#endif
#endif

/*
 * Simplified macro to send an error log message using the current LOG_TAG.
 */
#ifndef ALOGE
#if LOG_NDEBUG
#define ALOGE(format, ...)   ((void)0)
#else
#define ALOGE(format, ...) \
	printf("ALOGE [%s]: LINE: %d: "format"\n", \
	 	LOG_TAG, __LINE__, ##__VA_ARGS__);
#endif
#endif

// ---------------------------------------------------------------------
#ifndef LOG_ALWAYS_FATAL_IF
#if LOG_NDEBUG
#define LOG_ALWAYS_FATAL_IF(cond, format, ...)   ((void)0)
#else
#define LOG_ALWAYS_FATAL_IF(cond, format, ...) \
    if((cond)){                              \
        printf("LOG_ALWAYS_FATAL_IF [%s]: LINE: %d: "format"\n", \
	 	LOG_TAG, __LINE__, ##__VA_ARGS__);	\
    }
#endif
#endif

#ifndef LOG_FATAL_IF
#if LOG_NDEBUG
#define LOG_FATAL_IF(cond, format, ...)   ((void)0)
#else
#define LOG_FATAL_IF(cond, format, ...) \
    if((cond)){                              \
         printf("LOG_FATAL_IF [%s]: LINE: %d: "format"\n", \
	 	LOG_TAG, __LINE__, ##__VA_ARGS__);	\
    }
#endif
#endif

#ifndef ALOG_ASSERT
#if LOG_NDEBUG
#define ALOG_ASSERT(cond, format, ...)   ((void)0)
#else
#define ALOG_ASSERT(cond, format, ...) \
    if(!(cond)){                              \
         printf("ALOG_ASSERT [%s]: LINE: %d: "format"\n", \
	 	LOG_TAG, __LINE__, ##__VA_ARGS__);	\
    }
#endif
#endif

#ifndef ALOG
#if LOG_NDEBUG
#define ALOG(priority, tag, format, ...)   ((void)0)
#else
#define ALOG(priority, tag, format, ...) \
     printf("ALOG [%s]: LINE: %d: "format"\n", \
	 	LOG_TAG, __LINE__, ##__VA_ARGS__);
#endif
#endif

#ifdef __cplusplus
}
#endif

#endif // _LIBS_UTILS_LOG_H
