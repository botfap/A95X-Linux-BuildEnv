LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_ARM_MODE := arm
LOCAL_MODULE_TAGS := optional

LIBPLAYER_PATH=$(TOP)/packages/amlogic/LibPlayer/
LOCAL_SRC_FILES := \
	hls_download.c\
    hls_bandwidth_measure.c
	
LOCAL_C_INCLUDES := \
	$(TOP)/frameworks/native/include\
	$(LIBPLAYER_PATH)/amffmpeg\
	$(LOCAL_PATH)/../common

LOCAL_STATIC_LIBRARIES +=libhls_common
LOCAL_SHARED_LIBRARIES +=libamplayer
ifeq ($(TARGET_ARCH),arm)
    LOCAL_CFLAGS += -Wno-psabi
endif

LOCAL_MODULE := libhls_http

include $(BUILD_STATIC_LIBRARY)
