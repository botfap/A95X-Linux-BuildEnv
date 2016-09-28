LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_SRC_FILES := $(notdir $(wildcard $(LOCAL_PATH)/*.c)) 		

LOCAL_C_INCLUDES := $(TOP)/external/curl/include \
        $(LOCAL_PATH)/../../../amavutils/include \
        $(LOCAL_PATH)/../../../amffmpeg \
        $(LOCAL_PATH)/../include

LOCAL_STATIC_LIBRARIES += libcurl_base libcurl_common
LOCAL_SHARED_LIBRARIES += libcurl libutils libamavutils libamplayer liblog

LOCAL_MODULE := libcurl_mod
LOCAL_MODULE_TAGS := optional

LOCAL_ARM_MODE := arm
LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_PATH:=$(TARGET_OUT_SHARED_LIBRARIES)/amplayer
include $(BUILD_SHARED_LIBRARY)
