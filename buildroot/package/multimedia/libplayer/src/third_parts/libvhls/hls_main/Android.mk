LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_ARM_MODE := arm
LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES :=  hls_m3uparser.c \
	hls_m3ulivesession.c\
	hls_fifo.c\
	hls_simple_cache.c

LOCAL_C_INCLUDES := \
    $(TOP)/external/openssl/include\
    $(LOCAL_PATH)/../common\
    $(LOCAL_PATH)/../downloader\


LOCAL_MODULE := libhls

ifeq ($(TARGET_ARCH),arm)
    LOCAL_CFLAGS += -Wno-psabi
endif
include $(BUILD_STATIC_LIBRARY)
