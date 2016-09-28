LOCAL_PATH := $(call my-dir)

#
# Compile libmms
#
include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_PRELINK_MODULE := false
LOGLEVELS := ERROR WARN INFO DEBUG VERBOSE
cflags_loglevels := $(foreach ll,$(LOGLEVELS),-DAACD_LOGLEVEL_$(ll))

LOCAL_MODULE    := libmms
LOCAL_SRC_FILES := src/mms.c \
	src/mmsh.c \
	src/mmsx.c \
	src/uri.c 
LOCAL_CFLAGS	:= -DHAVE_CONFIG_H $(cflags_loglevels) 
LOCAL_CFLAGS	+= -DDEBUG
LOCAL_C_INCLUDES := $(LOCAL_PATH)/include/\
	 $(LOCAL_PATH)/../libiconv-1.14/include
LOCAL_SHARED_LIBRARIES := libiconv libutils liblog
include $(BUILD_SHARED_LIBRARY)


#
# Compile static libmms
#
include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_PRELINK_MODULE := false
LOGLEVELS := ERROR WARN INFO DEBUG VERBOSE
cflags_loglevels := $(foreach ll,$(LOGLEVELS),-DAACD_LOGLEVEL_$(ll))

LOCAL_MODULE    := libmms
LOCAL_SRC_FILES := src/mms.c \
	src/mmsh.c \
	src/mmsx.c \
	src/uri.c 
LOCAL_CFLAGS	:= -DHAVE_CONFIG_H $(cflags_loglevels) 
LOCAL_CFLAGS	+= -DDEBUG
LOCAL_C_INCLUDES := $(LOCAL_PATH)/include/\
	 $(LOCAL_PATH)/../libiconv-1.14/include
LOCAL_SHARED_LIBRARIES := libutils liblog
LOCAL_STATIC_LIBRARIES := libiconv 
include $(BUILD_STATIC_LIBRARY)
#
# Compile testcase
#
include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := tests
LOCAL_SRC_FILES := test/testdownload.c
LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/include
	
LOCAL_ARM_MODE := arm
LOCAL_MODULE := mms_testdnld
LOCAL_SHARED_LIBRARIES := libutils liblog
LOCAL_STATIC_LIBRARIES := libmms libiconv
include $(BUILD_EXECUTABLE)

#
#Compile sub-directory
#

include $(call all-makefiles-under,$(LOCAL_PATH))
