/******
*  init date: 2013.1.23
*  author: senbai.tao<senbai.tao@amlogic.com>
*  description: curl module register in amavutils
******/

#include<ammodule.h>
#include "libavformat/url.h"
#include "curl_log.h"
#include "curl/curl.h"

ammodule_methods_t  libcurl_module_methods;

ammodule_t AMPLAYER_MODULE_INFO_SYM = {
tag:
    AMPLAYER_MODULE_TAG,
version_major:
    AMPLAYER_API_MAIOR,
version_minor:
    AMPLAYER_API_MINOR,
    id: 0,
name: "curl_mod"
    ,
author: "Amlogic"
    ,
descript: "libcurl module binding library"
    ,
methods:
    &libcurl_module_methods,
dso :
    NULL,
reserved :
    {0},
};

extern URLProtocol ff_curl_protocol;

int libcurl_mod_init(const struct ammodule_t* module, int flags)
{
    CLOGI("libcurl module init\n");
    char * ver = NULL;
    ver = curl_version();
    CLOGI("curl version : [%s]", ver);
    curl_global_init(CURL_GLOBAL_ALL);
    av_register_protocol(&ff_curl_protocol);
    return 0;
}

int libcurl_mod_release(const struct ammodule_t* module)
{
    CLOGI("libcurl module release\n");
    curl_global_cleanup();
    return 0;
}

ammodule_methods_t  libcurl_module_methods = {
    .init           =   libcurl_mod_init,
    .release        =   libcurl_mod_release,
} ;

