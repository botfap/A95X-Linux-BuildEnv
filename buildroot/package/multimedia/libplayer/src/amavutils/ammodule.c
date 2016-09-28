/*
 * Copyright (C) 2008 The Android Open Source Project
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

#include <ammodule.h>
#include <dlfcn.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <limits.h>
#include <unistd.h>
#define LOG_TAG "ammodule"
#ifdef ANDROID
#include <utils/Log.h>
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#define  LOGV(...)
#else
#define LOGI printf
#define LOGE printf
#define LOGV printf
#endif
#include <amconfigutils.h>
/** Base path of the hal modules */
#define AM_LIBRARY_PATH1    "/system/lib/amplayer"
#define AM_LIBRARY_PATH2    "/vendor/lib/amplayer"
#define AM_LIBRARY_SETTING  "media.libplayer.modulepath"

static const char *defaut_path[] = {
    AM_LIBRARY_PATH1,
    AM_LIBRARY_PATH2,
    ""/*real path.*/

};

static const int PATH_COUNT =
    (sizeof(defaut_path) / sizeof(defaut_path[0]));

/**
 * Load the file defined by the variant and if successful
 * return the dlopen handle and the hmi.
 * @return 0 = success, !0 = failure.
 */
static int amload(const char *path,
                  const struct ammodule_t **pHmi)
{
    int status;
    void *handle;
    struct ammodule_t *hmi;

    /*
     * load the symbols resolving undefined symbols before
     * dlopen returns. Since RTLD_GLOBAL is not or'd in with
     * RTLD_NOW the external symbols will not be global
     */
    handle = dlopen(path, RTLD_NOW);
    if (handle == NULL) {
        char const *err_str = dlerror();
        LOGE("amload: module=%s\n%s", path, err_str ? err_str : "unknown");
        status = -EINVAL;
        goto done;
    }

    /* Get the address of the struct hal_module_info. */
    const char *sym = AMPLAYER_MODULE_INFO_SYM_AS_STR;
    hmi = (struct ammodule_t *)dlsym(handle, sym);
    if (hmi == NULL) {
        LOGE("amload: couldn't find symbol %s", sym);
        status = -EINVAL;
        goto done;
    }

    hmi->dso = handle;

    /* success */
    status = 0;
    if (hmi->tag != AMPLAYER_MODULE_TAG ||
        hmi->version_major != AMPLAYER_API_MAIOR) {
        status = -1;
        LOGE("module tag,api unsupport tag=%d,expect=%d api=%d.%d,expect=%d.%d\n",
             hmi->tag, AMPLAYER_MODULE_TAG,
             hmi->version_major, hmi->version_minor,
             AMPLAYER_API_MAIOR, AMPLAYER_API_MINOR);
    }
done:
    if (status != 0) {
        hmi = NULL;
        if (handle != NULL) {
            dlclose(handle);
            handle = NULL;
        }
    } else {
        LOGV("loaded module path=%s hmi=%p handle=%p",
             path, *pHmi, handle);
    }

    *pHmi = hmi;

    return status;
}

int ammodule_load_module(const char *modulename, const struct ammodule_t **module)
{
    int status = -ENOENT;;
    int i;
    const struct ammodule_t *hmi = NULL;
    char prop[PATH_MAX];
    char path[PATH_MAX];
    char name[PATH_MAX];
    const char *prepath = NULL;

    snprintf(name, PATH_MAX, "%s",  modulename);

    for (i = -1 ; i < PATH_COUNT; i++) {
        if (i >= 0) {
            prepath = defaut_path[i];
        } else {
            if (am_getconfig(AM_LIBRARY_SETTING, prop, NULL) <= 0) {
                continue;
            }
            prepath = prop;
        }
        snprintf(path, sizeof(path), "%s/lib%s.so",
                 prepath, name);
        if (access(path, R_OK) == 0) {
            break;
        }
        snprintf(path, sizeof(path), "%s/%s.so",
                 prepath, name);
        if (access(path, R_OK) == 0) {
            break;
        }

        snprintf(path, sizeof(path), "%s/%s",
                 prepath, name);
        if (access(path, R_OK) == 0) {
            break;
        }
        snprintf(path, sizeof(path), "%s",
                 name);
        if (access(path, R_OK) == 0) {
            break;
        }
    }

    status = -ENOENT;
    if (i < PATH_COUNT) {
        /* load the module, if this fails, we're doomed, and we should not try
         * to load a different variant. */
        status = amload(path, module);
    }
    LOGI("load mode %s,on %s %d\n", modulename, path, status);
    return status;
}

int ammodule_open_module(struct ammodule_t *module)
{
    int ret = -1000;

    if (module->methods) {
        ret = module->methods->init(module, 0);
    }
    if (ret != 0) {
        LOGE("open module (%s)  failed ret(%d)\n", module->name, ret);
    } else {
        LOGI("open module success,\n\tname:%s\n\t%s\n", module->name, module->descript);
    }
    return 0;
}
int ammodule_match_check(const char* filefmtstr,const char* fmtsetting)
{
        const char * psets=fmtsetting;
        const char *psetend;
        int psetlen=0;
        char codecstr[64]="";
		if(filefmtstr==NULL || fmtsetting==NULL)
			return 0;

        while(psets && psets[0]!='\0'){
                psetlen=0;
                psetend=strchr(psets,',');
                if(psetend!=NULL && psetend>psets && psetend-psets<64){
                        psetlen=psetend-psets;
                        memcpy(codecstr,psets,psetlen);
                        codecstr[psetlen]='\0';
                        psets=&psetend[1];//skip ";"
                }else{
                        strcpy(codecstr,psets);
                        psets=NULL;
                }
                if(strlen(codecstr)>0){
                        if(strstr(filefmtstr,codecstr)!=NULL)
                                return 1;
                }
        }
        return 0;
}

int  ammodule_simple_load_module(char* name){
    int ret; 
    struct ammodule_t *module;
    ret=ammodule_load_module(name,&module);
    if(ret==0){
        ret = ammodule_open_module(module);	
    }
    return ret;
        
}