LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE    := testlibplayer
LOCAL_MODULE_TAGS := tests
LOCAL_SRC_FILES := testlibplayer.c
LOCAL_ARM_MODE := arm
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../amplayer/player/include \
    $(LOCAL_PATH)/../amcodec/include \
    $(LOCAL_PATH)/../amadec/include \
    $(LOCAL_PATH)/../amffmpeg \
    $(LOCAL_PATH)/../amavutils/include \
    $(JNI_H_INCLUDE) 

LOCAL_STATIC_LIBRARIES := libamplayer libamplayer libamcodec libavformat libavcodec libavutil libamadec libamavutils
LOCAL_SHARED_LIBRARIES += libutils libmedia libbinder libz libdl libcutils

include $(BUILD_EXECUTABLE)

