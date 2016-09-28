LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
include $(LOCAL_PATH)/../common.mk
LOCAL_SRC_FILES := $(FFFILES)
LOCAL_C_INCLUDES :=		\
	$(LOCAL_PATH)		\
	$(LOCAL_PATH)/..	\
	$(LOCAL_PATH)/../../amavutils/include/	\
	$(LOCAL_PATH)/../../third_parts/rtmpdump	\
	external/zlib
LOCAL_CFLAGS += $(FFCFLAGS)
LOCAL_MODULE := $(FFNAME)
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
include $(LOCAL_PATH)/../common.mk
LOCAL_SRC_FILES := $(FFFILES)
LOCAL_C_INCLUDES :=		\
	$(LOCAL_PATH)		\
	$(LOCAL_PATH)/..	\
	$(LOCAL_PATH)/../../amavutils/include/	\
	$(LOCAL_PATH)/../../third_parts/rtmpdump	\
	external/zlib
LOCAL_CFLAGS += $(FFCFLAGS)
LOCAL_MODULE := $(FFNAME)
LOCAL_SHARED_LIBRARIES += librtmp  libutils libmedia libz libbinder libdl libcutils libc libavutil libavcodec libamavutils
LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_TAGS := optional
include $(BUILD_SHARED_LIBRARY)

# Reset CC as it's overwritten by common.mk
CC := $(HOST_CC)
