
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(notdir $(wildcard $(LOCAL_PATH)/*.c))

LOCAL_C_INCLUDES := $(LOCAL_PATH)/include\
    $(LOCAL_PATH)/../amavutils/include \
     $(JNI_H_INCLUDE)


LOCAL_SHARED_LIBRARIES += libutils libmedia libz libbinder
LOCAL_SHARED_LIBRARIES += libandroid_runtime   libnativehelper
LOCAL_SHARED_LIBRARIES += libcutils libc libamavutils

LOCAL_MODULE := libamvdec
LOCAL_MODULE_TAGS := optional
LOCAL_ARM_MODE := arm
LOCAL_PRELINK_MODULE := false
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(notdir $(wildcard $(LOCAL_PATH)/*.c))

LOCAL_C_INCLUDES := $(LOCAL_PATH)/include\
    $(LOCAL_PATH)/../amavutils/include \
     $(JNI_H_INCLUDE)


LOCAL_SHARED_LIBRARIES += libutils libmedia libz libbinder
LOCAL_SHARED_LIBRARIES += libandroid_runtime   libnativehelper
LOCAL_SHARED_LIBRARIES += libcutils libc libamavutils

LOCAL_MODULE := libamvdec
LOCAL_MODULE_TAGS := optional
LOCAL_ARM_MODE := arm
LOCAL_PRELINK_MODULE := false
include $(BUILD_SHARED_LIBRARY)