
#ifndef STREAMSOURCE_HH_SS
#define STREAMSOURCE_HH_SS
#ifdef ANDROID
#include <android/log.h>
#ifndef LOG_TAG
#define LOG_TAG "streamsource"
#endif
#undef LOGI
#undef LOGE
#undef LOGV
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#define  LOGV(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#else
#include <stdio.h>
#define LOGV printf
#define LOGD printf
#define LOGI printf
#define LOGW printf
#define LOGE printf
#endif
#define DTRACE()    LOGI("===%s===%d==\n",__FUNCTION__,__LINE__)
#endif

