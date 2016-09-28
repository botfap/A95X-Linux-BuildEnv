LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE    := libape
LOCAL_SRC_FILES := $(notdir $(wildcard $(LOCAL_PATH)/*.c))
LOCAL_ARM_MODE := arm
LOCAL_C_INCLUDES := $(LOCAL_PATH) \

ifeq ($(ARCH_ARM_HAVE_NEON),true)
	LOCAL_CFLAGS += -D__ARM_HAVE_NEON -DOPT_NEON
endif

include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_SHARED_LIBRARIES += libutils libmedia libz libbinder libdl libcutils libc 

LOCAL_MODULE    := libape
LOCAL_SRC_FILES := $(notdir $(wildcard $(LOCAL_PATH)/*.c))
LOCAL_ARM_MODE := arm
LOCAL_MODULE_TAGS := optional
LOCAL_C_INCLUDES := $(LOCAL_PATH) \
LOCAL_PRELINK_MODULE := false 

ifeq ($(ARCH_ARM_HAVE_NEON),true)
	LOCAL_CFLAGS += -D__ARM_HAVE_NEON -DOPT_NEON
endif

include $(BUILD_SHARED_LIBRARY)
