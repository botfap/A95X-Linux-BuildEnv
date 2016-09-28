LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_SRC_FILES := $(notdir $(wildcard $(LOCAL_PATH)/*.c)) 		

LOCAL_C_INCLUDES := $(TOP)/external/curl/include \
        $(LOCAL_PATH)/../include

LOCAL_SHARED_LIBRARIES += libcurl

LOCAL_MODULE := libcurl_base
LOCAL_MODULE_TAGS := optional

LOCAL_ARM_MODE := arm
LOCAL_PRELINK_MODULE := false
include $(BUILD_STATIC_LIBRARY)
