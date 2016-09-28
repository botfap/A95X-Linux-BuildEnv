LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SHARED_LIBRARIES +=  libcutils libc  liblog 
LOCAL_MODULE    := libpcm_wfd
LOCAL_SRC_FILES := lpcm_decode.c
#$(notdir $(wildcard $(LOCAL_PATH)/*.c))
LOCAL_ARM_MODE := arm
LOCAL_MODULE_TAGS := optional
LOCAL_C_INCLUDES := $(LOCAL_PATH)  $(LOCAL_PATH)/../../amadec/include $(LOCAL_PATH)/../../amadec/

include $(BUILD_SHARED_LIBRARY)
