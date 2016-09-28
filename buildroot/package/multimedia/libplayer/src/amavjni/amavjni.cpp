

#define LOG_NDEBUG 0
#define LOG_TAG "AmavjniC"

#include "jni.h"
#include "JNIHelp.h"

#include <utils/misc.h>
#include <android_runtime/AndroidRuntime.h>

#include <utils/Log.h>
#include <stdio.h>
#include <stdlib.h>

#include <player.h>

#include <player_set_sys.h>

#include <dlfcn.h>

#ifndef LOGI
    #define LOGV ALOGV
    #define LOGD ALOGD
    #define LOGI ALOGI
    #define LOGW ALOGW
    #define LOGE ALOGE
#endif

#define TRACE() LOGI("[%s::%d]\n",__FUNCTION__,__LINE__)
using namespace android;

///namespace android{
struct amplayer_mgt {
    play_control_t  play_ctrl;
    int             player_pid;
    int             init_steps;
    jclass          mClass;
    jobject         mObject;
    int             mfulltime_ms;
    int             mcurrent_ms;
};

struct fields_t {
    JavaVM      *gJavaVM ;
    JNIEnv* env;
    jmethodID   post_event;
};

static struct fields_t fields;


static jint com_amlogic_libplayer_amavjni_globalinit(JNIEnv* env, jobject thiz)
{
    static int inited = 0;
    int ret = 0;
    TRACE();
    if (!inited) {
        ret = player_init();
        if (ret != 0) {
            return -1;
        }
        jclass clazz;
        clazz = env->FindClass("com/amlogic/libplayer/amavjni");
        if (clazz == NULL) {
            jniThrowException(env, "java/lang/RuntimeException", "com/amlogic/libplayer/amavjni");
            return -2;
        }
        fields.post_event = env->GetStaticMethodID(clazz, "postEventFromNative",
                            "(Ljava/lang/Object;IIILjava/lang/Object;)V");
        if (fields.post_event == NULL) {
            jniThrowException(env, "java/lang/RuntimeException", "Can't find postEventFromNative");
            return -3;
        }
    }
    inited = !ret;
    return ret;
}

int com_amlogic_libplayer_amavjni_notify(struct amplayer_mgt * player, int msg, int ext1, int ext2)
{
    JNIEnv *env = NULL;
    int isAttached = -1;
    int ret = -1;
    ret = fields.gJavaVM->GetEnv((void**)&env, JNI_VERSION_1_4);
    if (ret < 0) {
        //LOGE("callback handler:failed to get java env by native thread");
        ret = (fields.gJavaVM)->AttachCurrentThread(&env, NULL);
        if (ret < 0) {
            LOGE("notifly handler:failed to attach current thread");
            return -2;
        }
        isAttached = 1;

    }
    if (env == NULL || player->mClass == NULL || fields.post_event == NULL) {
        LOGI("com_amlogic_libplayer_amavjni_notify,env=%p,class=%p,post=%p\n", env, player->mClass, fields.post_event);
        return -3;
    }

    env->CallStaticVoidMethod(player->mClass, fields.post_event, player->mObject, msg, ext1, ext2, 0);
    if (isAttached > 0) {
        LOGI("callback handler:detach current thread");
        (fields.gJavaVM)->DetachCurrentThread();
    }
    return ret;

}


static int player_notify_handle(int pid, int msg, unsigned long ext1, unsigned long ext2)
{
    struct amplayer_mgt *player = (struct amplayer_mgt *)player_get_extern_priv(pid);
    player_info_t *info;
    switch (msg) {
    case PLAYER_EVENTS_PLAYER_INFO:
        info = (player_info_t *)ext1;
        if (info != NULL && player != NULL) {
            player->mfulltime_ms = info->full_time * 1000;
            player->mcurrent_ms = info->current_ms;
        }
    default:
        com_amlogic_libplayer_amavjni_notify(player, msg, ext1, ext2);
        break;
    }

    LOGI("pi=%d-msg=%d,ext1=%ld,ext2=%ld\n", pid, msg, ext1, ext2);
    return 0;
}
static jint com_amlogic_libplayer_amavjni_newplayer(JNIEnv* env, jobject thiz, jobject weak_thiz)
{
    struct amplayer_mgt *player;

    player = (struct amplayer_mgt *)malloc(sizeof(struct amplayer_mgt));
    if (player == NULL) {
        return -ENOMEM;
    }
    TRACE();
    player_init();
    memset(player, 0, sizeof(*player));
    player->play_ctrl.audio_index = -1;
    player->play_ctrl.video_index = -1;
    player->play_ctrl.sub_index = -1;
    player->play_ctrl.t_pos = -1;
    player->play_ctrl.need_start = 1;
    player->player_pid = -1;
    player->play_ctrl.callback_fn.notify_fn = player_notify_handle;
    player->play_ctrl.callback_fn.update_interval = 1000;

    jclass clazz = env->GetObjectClass(thiz);
    if (clazz == NULL) {
        LOGE("Can't find amplayer");
        free(player);
        jniThrowException(env, "java/lang/Exception", NULL);
        return -1;
    }
    player->mClass = (jclass)env->NewGlobalRef(clazz);
    if (clazz == NULL) {
        LOGE("Can't find mClass");
        free(player);
        jniThrowException(env, "java/lang/Exception", NULL);
        return -1;
    }
    player->mObject  = env->NewGlobalRef(weak_thiz);
    if (clazz == NULL) {
        LOGE("Can't find mObject");
        free(player);
        jniThrowException(env, "java/lang/Exception", NULL);
        return -1;
    }


    return (jint)player;
}

static jint com_amlogic_libplayer_amavjni_setdatasource(JNIEnv* env, jobject clazz, jint pplayer, jstring jpath, jstring jheaders)
{
    const char *path = NULL;
    const char *headers = NULL;
    struct amplayer_mgt *player = (struct amplayer_mgt *)pplayer;
    TRACE();
    if (player == NULL || jpath == NULL) {
        return -1;
    }

    path = env->GetStringUTFChars(jpath, NULL);
    if (jheaders != NULL) {
        headers = env->GetStringUTFChars(jheaders, NULL);
    };
    if (path != NULL) {
        path = strdup(path);
    }
    if (headers != NULL) {
        headers = strdup(headers);
    }
    player->play_ctrl.file_name = (char *)path;
    player->play_ctrl.headers = (char *)headers;
    player->play_ctrl.need_start = 1;
    return 0;
}

static int player_sysfs_set(struct amplayer_mgt *player, const char * props)
{
    TRACE();
    char path[1024] = "";
    char val[1024] = "";
    char *pstr;
    LOGI("player_sysfs_set:%s\n", props);
    if (props == NULL) {
        return -1;
    }
    pstr = strchr(props, '=');
    if (pstr == NULL) {
        return -1;
    }
    strncpy(path, props, pstr - props);
    strcpy(val, pstr + 1);
    if (path[0] == '\0' || val[0] == '\0') {
        return -1;
    }
    LOGI("set sysfs:%s=%s\n", path, val);
    set_sysfs_str(path, val);
    return 0;
}
static jint com_amlogic_libplayer_amavjni_sysset(JNIEnv* env, jobject clazz, jint pplayer, jstring props)
{
    struct amplayer_mgt *player = (struct amplayer_mgt *)pplayer;
    const char *propstr = NULL;
    if (player == NULL || props == NULL) {
        return -1;
    }
    TRACE();
    propstr = env->GetStringUTFChars(props, NULL);
    if (propstr != NULL) {
        return player_sysfs_set(player, propstr);
    }
    return 0;
}

static int player_cmd(struct amplayer_mgt *player, const char * props)
{
    TRACE();
#define IS_CMD(str,cmd) (strncmp(str,cmd,strlen(cmd))==0)
    if (IS_CMD(props, "AMPLAYER:VIDEO_ON")) {
        player_video_overlay_en(1);
    } else if (IS_CMD(props, "AMPLAYER:VIDEO_OFF")) {
        player_video_overlay_en(0);
    }
    return 0;
}


static jint com_amlogic_libplayer_amavjni_playercmd(JNIEnv* env, jobject clazz, jint pplayer, jstring props)
{
    struct amplayer_mgt *player = (struct amplayer_mgt *)pplayer;
    const char *propstr = NULL;
    if (player == NULL || props == NULL) {
        return -1;
    }
    TRACE();
    propstr = env->GetStringUTFChars(props, NULL);
    if (propstr != NULL) {
        return player_cmd(player, propstr);
    }
    return 0;
}


static jint com_amlogic_libplayer_amavjni_prepare_async(JNIEnv* env, jobject clazz, jint pplayer)
{
    struct amplayer_mgt *player = (struct amplayer_mgt *)pplayer;
    int pid;
    if (player == NULL) {
        return -1;
    }
    TRACE();
    pid = player_start(&player->play_ctrl, (unsigned long)player);
    player->player_pid = pid;
    return pid;
}


static jint com_amlogic_libplayer_amavjni_prepare(JNIEnv* env, jobject clazz, jint pplayer)
{
    int pid;
    TRACE();
    pid = com_amlogic_libplayer_amavjni_prepare_async(env, clazz, pplayer);
    if (pid < 0) {
        return -1;
    }
    while (player_get_state(pid) != PLAYER_INITOK) {
        if ((player_get_state(pid)) == PLAYER_ERROR ||
            player_get_state(pid) == PLAYER_STOPED ||
            player_get_state(pid) == PLAYER_PLAYEND ||
            player_get_state(pid) == PLAYER_EXIT) {
            return -1;
        }
        usleep(1000 * 10);
    }
    return pid;
}

static jint com_amlogic_libplayer_amavjni_start(JNIEnv* env, jobject clazz, jint pplayer)
{
    struct amplayer_mgt *player = (struct amplayer_mgt *)pplayer;
    TRACE();
    player_start_play(player->player_pid);
    player_resume(player->player_pid);
    return 0;
}


static jint com_amlogic_libplayer_amavjni_stop(JNIEnv* env, jobject clazz, jint pplayer)
{
    struct amplayer_mgt *player = (struct amplayer_mgt *)pplayer;
    TRACE();
    player_stop(player->player_pid);
    return 0;
}

static jint com_amlogic_libplayer_amavjni_pause(JNIEnv* env, jobject clazz, jint pplayer)
{
    struct amplayer_mgt *player = (struct amplayer_mgt *)pplayer;
    TRACE();
    player_pause(player->player_pid);
    return 0;
}

static jint com_amlogic_libplayer_amavjni_release(JNIEnv* env, jobject clazz, jint pplayer)
{
    struct amplayer_mgt *player = (struct amplayer_mgt *)pplayer;
    TRACE();
    player_exit(player->player_pid);
    if (player->mClass != NULL) {
        env->DeleteGlobalRef(player->mClass);
    }
    if (player->mObject != NULL) {
        env->DeleteGlobalRef(player->mObject);
    }
    free(player);
    return 0;
}
static jint com_amlogic_libplayer_amavjni_getstate(JNIEnv* env, jobject clazz, jint pplayer)
{
    struct amplayer_mgt *player = (struct amplayer_mgt *)pplayer;
    TRACE();
    return player_get_state(player->player_pid);
}

static jint com_amlogic_libplayer_amavjni_getduration(JNIEnv* env, jobject clazz, jint pplayer)
{
    struct amplayer_mgt *player = (struct amplayer_mgt *)pplayer;
    TRACE();
    return player->mfulltime_ms;
}
static jint com_amlogic_libplayer_amavjni_getcurrenttime(JNIEnv* env, jobject clazz, jint pplayer)
{
    struct amplayer_mgt *player = (struct amplayer_mgt *)pplayer;
    TRACE();
    return player->mcurrent_ms;
}
static jint com_amlogic_libplayer_amavjni_seekto(JNIEnv* env, jobject clazz, jint pplayer, jint time_ms)
{
    struct amplayer_mgt *player = (struct amplayer_mgt *)pplayer;
    TRACE();
    if (time_ms > 0 && time_ms < player->mfulltime_ms) {
        time_ms = time_ms / 1000;
        return player_timesearch(player->player_pid, time_ms);
    }
    return -1;
}

static jint com_amlogic_libplayer_amavjni_fffb(JNIEnv* env, jobject clazz, jint pplayer, jint speed)
{
    struct amplayer_mgt *player = (struct amplayer_mgt *)pplayer;
    TRACE();
    if (speed > 0) {
        return player_forward(player->player_pid, speed);
    } else if (speed < 0) {
        return player_backward(player->player_pid, -speed);
    } else {
        return player_backward(player->player_pid, 0); /*to normal player*/
    }
    return -1;
}









static JNINativeMethod gAmavjnimthods[] = {
    /* name, signature, funcPtr */
    { "native_globalinit",          "()I", (void*) com_amlogic_libplayer_amavjni_globalinit },
    { "native_newplayer",           "(Ljava/lang/Object;)I", (void*) com_amlogic_libplayer_amavjni_newplayer },
    { "native_setdatasource",       "(ILjava/lang/String;Ljava/lang/String;)I", (void*) com_amlogic_libplayer_amavjni_setdatasource },
    { "native_sysset",              "(ILjava/lang/String;)I", (void*) com_amlogic_libplayer_amavjni_sysset },
    { "native_playercmd",           "(ILjava/lang/String;)I", (void*) com_amlogic_libplayer_amavjni_playercmd },
    { "native_prepare",             "(I)I", (void*) com_amlogic_libplayer_amavjni_prepare},
    { "native_prepare_async",       "(I)I", (void*) com_amlogic_libplayer_amavjni_prepare_async},
    { "native_start",               "(I)I", (void*) com_amlogic_libplayer_amavjni_start },
    { "native_stop",                "(I)I", (void*) com_amlogic_libplayer_amavjni_stop },
    { "native_pause",               "(I)I", (void*) com_amlogic_libplayer_amavjni_pause },
    { "native_release",             "(I)I", (void*) com_amlogic_libplayer_amavjni_release },
    { "native_getstate",            "(I)I", (void*) com_amlogic_libplayer_amavjni_getstate },
    { "native_getduration",         "(I)I", (void*) com_amlogic_libplayer_amavjni_getduration },
    { "native_getcurrenttime",      "(I)I", (void*) com_amlogic_libplayer_amavjni_getcurrenttime },
    { "native_seekto",              "(II)I", (void*) com_amlogic_libplayer_amavjni_seekto },
    { "native_fffb",                "(II)I", (void*) com_amlogic_libplayer_amavjni_fffb }
};


int register_amplayer_amavjni(JNIEnv* env)
{
    int ret;
    TRACE();
    ret = android::AndroidRuntime::registerNativeMethods(env,
            "com/amlogic/libplayer/amavjni", gAmavjnimthods, NELEM(gAmavjnimthods));
    return ret;
}







JNIEXPORT jint
JNI_OnLoad(JavaVM* vm, void* reserved)
{
    jint ret;
    JNIEnv* env = NULL;
    TRACE();
    if (vm->GetEnv((void**)&env, JNI_VERSION_1_4) != JNI_OK || NULL == env) {
        LOGE("GetEnv failed!");
        return -1;
    }
    fields.gJavaVM = vm;
    fields.env = env;
    TRACE();
    ret = register_amplayer_amavjni(env);
    LOGI("register_amplayer_amavjni=%d\n", ret);
    if (ret == 0) {
        return JNI_VERSION_1_4;
    } else {
        return -1;
    }
}

JNIEXPORT void
JNI_OnUnload(JavaVM* vm, void* reserved)
{
    return;
}
