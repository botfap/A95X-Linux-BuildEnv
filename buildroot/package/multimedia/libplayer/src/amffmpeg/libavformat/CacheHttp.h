/*
 * CacheHttp definitions
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */


#ifndef AVFORMAT_CACHEHTTP_H
#define AVFORMAT_CACHEHTTP_H

#include "url.h"

 int CacheHttp_Open(void ** handle,const char* headers,void* arg);
 int CacheHttp_Read(void * handle, uint8_t * cache, int size);
 int CacheHttp_Close(void * handle);
 int CacheHttp_GetSpeed(void * handle, int * arg1, int * arg2, int * arg3);
 int CacheHttp_Reset(void * handle, int cmf_flag);
 int CacheHttp_Seek(void * handle, int64_t pos);
 int CacheHttp_GetBuffedTime(void *handle);
 int CacheHttp_GetBufferPercentage(void *handle,int* per);//from 0~100
 int CacheHttp_GetEstimateBitrate(void *handle,int64_t* val);
  int CacheHttp_GetErrorCode(void *handle,int64_t* val);
 
 #endif