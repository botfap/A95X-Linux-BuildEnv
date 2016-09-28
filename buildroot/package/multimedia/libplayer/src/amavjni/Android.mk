
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(notdir $(wildcard $(LOCAL_PATH)/*.cpp)) 												

LOCAL_C_INCLUDES := $(LOCAL_PATH)/include\
	$(LOCAL_PATH)/../amplayer/player/include \
	$(LOCAL_PATH)/../amcodec/include \
	$(LOCAL_PATH)/../amadec/include \
	$(LOCAL_PATH)/../amffmpeg\
	 $(JNI_H_INCLUDE) 

LOCAL_STATIC_LIBRARIES := libamcodec libavformat libavcodec libavutil libamadec 
LOCAL_SHARED_LIBRARIES += libutils libmedia libz libbinder 
LOCAL_SHARED_LIBRARIES += libandroid_runtime   libnativehelper
LOCAL_SHARED_LIBRARIES += libamplayer libcutils libc

LOCAL_MODULE := libamavjni
LOCAL_MODULE_TAGS := optional
LOCAL_ARM_MODE := arm
LOCAL_PRELINK_MODULE := false
include $(BUILD_SHARED_LIBRARY)

