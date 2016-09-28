LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(notdir $(wildcard $(LOCAL_PATH)/*.c)) 									

LOCAL_SRC_FILES +=TsPlayer.cpp

LOCAL_C_INCLUDES := $(LOCAL_PATH)/include \
        $(LOCAL_PATH)/../../amcodec/include \
        $(LOCAL_PATH)/../../amadec/include \

LOCAL_STATIC_LIBRARIES :=   libamcodec libamadec
LOCAL_SHARED_LIBRARIES += libutils libmedia libz libbinder libdl libcutils libc

LOCAL_MODULE := libtsplayer
LOCAL_MODULE_TAGS := optional

LOCAL_ARM_MODE := arm
LOCAL_PRELINK_MODULE := false
include $(BUILD_SHARED_LIBRARY)



include $(CLEAR_VARS)
LOCAL_MODULE    := tsplayer
LOCAL_MODULE_TAGS := samples
LOCAL_SRC_FILES := main.cpp
LOCAL_ARM_MODE := arm
LOCAL_C_INCLUDES := $(LOCAL_PATH)/include \
        $(LOCAL_PATH)/../../amcodec/include \
        $(LOCAL_PATH)/../../amadec/include \

LOCAL_SHARED_LIBRARIES += libutils libmedia libbinder libz libdl libcutils libtsplayer

include $(BUILD_EXECUTABLE)



