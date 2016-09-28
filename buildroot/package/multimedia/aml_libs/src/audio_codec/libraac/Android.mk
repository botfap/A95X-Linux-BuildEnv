LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE    := libraac

LOCAL_SRC_FILES := $(notdir $(wildcard $(LOCAL_PATH)/*.c))
LOCAL_SRC_FILES += sbrcov.s\
                   sbrqmfak.s\
                   sbrqmfsk.s

LOCAL_CFLAGS := -DHAVE_NEON=1 -DHAVE_CONFIG -DOPT_NEON -DREAL_IS_FLOAT -mfloat-abi=softfp -mfpu=neon

LOCAL_ARM_MODE := arm 
LOCAL_C_INCLUDES := $(LOCAL_PATH)\
                    $(LOCAL_PATH)/include 

include $(BUILD_STATIC_LIBRARY)


#########################################################################
include $(CLEAR_VARS)

LOCAL_MODULE    := libraac

LOCAL_SRC_FILES := $(notdir $(wildcard $(LOCAL_PATH)/*.c))
LOCAL_SRC_FILES += sbrcov.s\
                   sbrqmfak.s\
                   sbrqmfsk.s

LOCAL_CFLAGS := -DHAVE_NEON=1 -DHAVE_CONFIG -DOPT_NEON -DREAL_IS_FLOAT -mfloat-abi=softfp -mfpu=neon

LOCAL_ARM_MODE := arm
LOCAL_MODULE_TAGS := optional
LOCAL_C_INCLUDES := $(LOCAL_PATH)\
                    $(LOCAL_PATH)/include 

LOCAL_SHARED_LIBRARIES += libutils libmedia libz libbinder libdl libcutils libc

LOCAL_PRELINK_MODULE := false 

include $(BUILD_SHARED_LIBRARY)
