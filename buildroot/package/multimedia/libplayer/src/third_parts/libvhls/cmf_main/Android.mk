LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_ARM_MODE := arm
LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := \
	hls_cmf_impl.c
	
LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/../hls_main\
    $(LOCAL_PATH)/../common\
    $(LOCAL_PATH)/../include

ifeq ($(TARGET_ARCH),arm)
    LOCAL_CFLAGS += -Wno-psabi
endif

LOCAL_MODULE := libhls_cmf

include $(BUILD_STATIC_LIBRARY)
