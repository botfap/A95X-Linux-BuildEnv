LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
include $(LOCAL_PATH)/../common.mk
LOCAL_SRC_FILES := $(FFFILES)
LOCAL_C_INCLUDES :=		\
	$(LOCAL_PATH)		\
	$(LOCAL_PATH)/..
LOCAL_CFLAGS += $(FFCFLAGS)
LOCAL_MODULE := $(FFNAME)
# Reset CC as it's overwritten by common.mk
include $(BUILD_STATIC_LIBRARY)


include $(CLEAR_VARS)
include $(LOCAL_PATH)/../common.mk
LOCAL_SRC_FILES := $(FFFILES)
LOCAL_C_INCLUDES :=		\
	$(LOCAL_PATH)		\
	$(LOCAL_PATH)/..	\
	external/zlib
LOCAL_CFLAGS += $(FFCFLAGS)
LOCAL_MODULE := $(FFNAME)
LOCAL_SHARED_LIBRARIES += libutils libmedia libz libbinder libdl libcutils libc 
LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_TAGS := optional
include $(BUILD_SHARED_LIBRARY)

CC := $(HOST_CC)

