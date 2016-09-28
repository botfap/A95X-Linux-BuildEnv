LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(notdir $(wildcard $(LOCAL_PATH)/*.c)) 	
LOCAL_SRC_FILES += $(notdir $(wildcard $(LOCAL_PATH)/*.cpp)) 

LOCAL_C_INCLUDES := $(LOCAL_PATH)/include \
     $(LOCAL_PATH)/../amcodec/include \
	 $(JNI_H_INCLUDE) \
	 $(TOP)/frameworks/native/services \
	 $(TOP)/frameworks/native/include 

LOCAL_SHARED_LIBRARIES += libutils
LOCAL_SHARED_LIBRARIES += libandroid_runtime   libnativehelper
LOCAL_SHARED_LIBRARIES +=  libcutils libc libdl
LOCAL_SHARED_LIBRARIES += libbinder \
                          libsystemwriteservice
LOCAL_MODULE := libamavutils
LOCAL_MODULE_TAGS := optional
LOCAL_ARM_MODE := arm
LOCAL_PRELINK_MODULE := false
include $(BUILD_SHARED_LIBRARY)


include $(CLEAR_VARS)


LOCAL_SRC_FILES := $(notdir $(wildcard $(LOCAL_PATH)/*.c)) 	
LOCAL_SRC_FILES += $(notdir $(wildcard $(LOCAL_PATH)/*.cpp))

LOCAL_C_INCLUDES := $(LOCAL_PATH)/include \
         $(LOCAL_PATH)/../amcodec/include \
         $(JNI_H_INCLUDE) \
         $(TOP)/frameworks/native/services \
         $(TOP)/frameworks/native/include 

LOCAL_SHARED_LIBRARIES += libutils
LOCAL_SHARED_LIBRARIES += libandroid_runtime   libnativehelper
LOCAL_SHARED_LIBRARIES +=  libcutils libc

LOCAL_MODULE := libamavutils
LOCAL_MODULE_TAGS := optional
LOCAL_ARM_MODE := arm
LOCAL_PRELINK_MODULE := false
include $(BUILD_STATIC_LIBRARY)

