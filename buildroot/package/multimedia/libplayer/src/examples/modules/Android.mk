LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_SRC_FILES := $(notdir $(wildcard $(LOCAL_PATH)/*.c)) 		

LOCAL_C_INCLUDES := $(LOCAL_PATH)/include \
        $(LOCAL_PATH)/../../amavutils/include \


LOCAL_SHARED_LIBRARIES += libutils libmedia libz libbinder libdl libcutils libc libamavutils

LOCAL_MODULE := libtestmodule
LOCAL_MODULE_TAGS := samples

LOCAL_ARM_MODE := arm
LOCAL_PRELINK_MODULE := false
include $(BUILD_SHARED_LIBRARY)