LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE    := kplayer
LOCAL_MODULE_TAGS := samples
LOCAL_SRC_FILES := kplayer.c
LOCAL_ARM_MODE := arm
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../amplayer/player/include \
    $(LOCAL_PATH)/../amcodec/include \
    $(LOCAL_PATH)/../amadec/include \
    $(LOCAL_PATH)/../amffmpeg \
    $(JNI_H_INCLUDE) \
    $(LOCAL_PATH)/../streamsource \

LOCAL_STATIC_LIBRARIES := libamplayer libamplayer libamcodec libavformat librtmp libavcodec libavutil libamadec libamavutils libamstreaming
LOCAL_SHARED_LIBRARIES += libutils libmedia libbinder libz libdl libcutils libssl libcrypto 

LOCAL_SHARED_LIBRARIES += libbinder  \
                          libsystemwriteservice
include $(BUILD_EXECUTABLE)


include $(CLEAR_VARS)
LOCAL_MODULE    := simple_player
LOCAL_MODULE_TAGS := samples
LOCAL_SRC_FILES := simple_player.c
LOCAL_ARM_MODE := arm
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../amplayer/player/include \
    $(LOCAL_PATH)/../amcodec/include \
    $(LOCAL_PATH)/../amadec/include \
    $(LOCAL_PATH)/../amffmpeg \
    $(JNI_H_INCLUDE) 

LOCAL_STATIC_LIBRARIES := libamplayer libamplayer libamcodec libavformat librtmp libavcodec libavutil libamadec libamavutils
LOCAL_SHARED_LIBRARIES += libutils libmedia libbinder libz libdl libcutils libssl libcrypto

LOCAL_SHARED_LIBRARIES += libbinder  \
                          libsystemwriteservice
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE    := esplayer
LOCAL_MODULE_TAGS := samples
LOCAL_SRC_FILES := esplayer.c
LOCAL_ARM_MODE := arm
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../amplayer/player/include \
    $(LOCAL_PATH)/../amcodec/include \
    $(LOCAL_PATH)/../amadec/include \
    $(LOCAL_PATH)/../amffmpeg \
    $(JNI_H_INCLUDE)

LOCAL_STATIC_LIBRARIES := libamplayer libamplayer libamcodec libavformat librtmp libavcodec libavutil libamadec libamavutils
LOCAL_SHARED_LIBRARIES += libutils libmedia libbinder libz libdl libcutils libssl libcrypto

LOCAL_SHARED_LIBRARIES += libbinder  \
                          libsystemwriteservice
include $(BUILD_EXECUTABLE)

