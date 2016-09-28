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
#ifndef LOG_PRINT_H
#define LOG_PRINT_H

#ifdef ANDROID
#include <android/log.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#define  LOG_TAG    "amadec"
#define adec_print(...) __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#else
#define LOG_DEFAULT  0
char *level;
#define adec_print(f,s...) do{level=getenv("LOG_LEVEL"); \
	                                   if(level&&atoi(level)>LOG_DEFAULT) \ 
						   fprintf(stderr,f,##s);\
						   else; }while(0);
#endif


#endif
