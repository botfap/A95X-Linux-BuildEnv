//by peter,2012,1121
#include<ammodule.h>
#include "libavformat/url.h"
#include <android/log.h>
#define  LOG_TAG    "libmms_mod"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)


ammodule_methods_t  libmms_module_methods;
ammodule_t AMPLAYER_MODULE_INFO_SYM = {
    tag: AMPLAYER_MODULE_TAG,
    version_major: AMPLAYER_API_MAIOR,
    version_minor: AMPLAYER_API_MINOR,
    id: 0,
    name: "mms_mod",
    author: "Amlogic",
    descript:"libmms module binding library",
    methods: &libmms_module_methods,
    dso : NULL,
    reserved : {0},
};

extern URLProtocol ff_mmsx_protocol;	
int libmms_mod_init(const struct ammodule_t* module, int flags)
{
    LOGI("libmms module init\n");
    av_register_protocol(&ff_mmsx_protocol);
    return 0;
}


int libmms_mod_release(const struct ammodule_t* module)
{
    LOGI("libmms module release\n");
    return 0;
}


ammodule_methods_t  libmms_module_methods={
   .init =	libmms_mod_init,
   .release =	libmms_mod_release,
} ;

