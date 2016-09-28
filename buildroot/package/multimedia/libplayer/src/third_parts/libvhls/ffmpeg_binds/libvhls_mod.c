//by peter,2012,1121
#include "ammodule.h"
#include "libavformat/url.h"
#include <android/log.h>
#define  LOG_TAG    "libvhls_mod"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)


ammodule_methods_t  libvhls_module_methods;
ammodule_t AMPLAYER_MODULE_INFO_SYM = {
    tag: AMPLAYER_MODULE_TAG,
    version_major: AMPLAYER_API_MAIOR,
    version_minor: AMPLAYER_API_MINOR,
    id: 0,
    name: "vhls_mod",
    author: "Amlogic",
    descript:"libvhls module binding library",
    methods: &libvhls_module_methods,
    dso : NULL,
    reserved : {0},
};

extern URLProtocol vhls_protocol;	
int libvhls_mod_init(const struct ammodule_t* module, int flags)
{
    LOGI("libvhls module init\n");
    av_register_protocol(&vhls_protocol);
    return 0;
}


int libvhls_mod_release(const struct ammodule_t* module)
{
    LOGI("libvhls module release\n");
    return 0;
}


ammodule_methods_t  libvhls_module_methods={
   .init =	libvhls_mod_init,
   .release =	libvhls_mod_release,
} ;

