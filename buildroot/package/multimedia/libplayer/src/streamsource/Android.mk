LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(notdir $(wildcard $(LOCAL_PATH)/*.c))

LOCAL_C_INCLUDES := $(LOCAL_PATH)/../amffmpeg/\
                     $(LOCAL_PATH)/../amavutils/include


LOCAL_ARM_MODE := arm
##LOCAL_STATIC_LIBRARIES :=
LOCAL_MODULE:= libamstreaming

include $(BUILD_STATIC_LIBRARY)


include $(CLEAR_VARS)

LOCAL_SRC_FILES :=$(notdir $(wildcard $(LOCAL_PATH)/*.c))

LOCAL_C_INCLUDES := $(LOCAL_PATH)/../amffmpeg/\
                     $(LOCAL_PATH)/../amavutils/include



#LOCAL_STATIC_LIBRARIES := 
LOCAL_SHARED_LIBRARIES := libamplayer libutils liblog

LOCAL_ARM_MODE := arm
LOCAL_MODULE:= libamstreaming
LOCAL_MODULE_TAGS := optional
LOCAL_PRELINK_MODULE := false
include $(BUILD_SHARED_LIBRARY)

