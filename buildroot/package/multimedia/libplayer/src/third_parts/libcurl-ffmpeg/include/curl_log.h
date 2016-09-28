#ifndef CURL_LOG_H_
#define CURL_LOG_H_

#undef CLOGI
#undef CLOGE
#undef CLOGV

#ifdef ANDROID
#include <android/log.h>
#ifndef LOG_TAG
#define LOG_TAG "curl-mod"
#endif
#define  CLOGI(...)  __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define  CLOGE(...)  __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define  CLOGV(...)  __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__)
#else
#define LOG_DEFAULT  0
char *level;
#define CLOGI(f,s...)  do{level=getenv("CURLLOG_LEVEL"); \
	                                   if(level&&atoi(level)>LOG_DEFAULT) \ 
						   fprintf(stderr,f,##s);\
						   else; }while(0);
#define CLOGE(f,s...) fprintf(stderr,f,##s)
#define CLOGV(f,s...) CLOGI(f,s...)
/*#define CLOGI(f,s...) fprintf(stderr,f,##s)
#define CLOGE(f,s...) fprintf(stderr,f,##s)
#define CLOGV(f,s...) fprintf(stderr,f,##s)*/
#endif

#endif