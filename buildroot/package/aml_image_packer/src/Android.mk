# 
# Copyright 2013 The Amlogic Iamge Pack Tool
#
# image pack tool
#

LOCAL_PATH:= $(call my-dir)

SRC_FILES := \
	crc32.c \
	sha1.c \
	amlImage.c \
	sparse_format.c

ifeq ($(strip $(USE_MINGW)),y)	
#################################

include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(SRC_FILES)
LOCAL_MODULE := libimagepack
LOCAL_CFLAGS := -DBUILD_LIBRARY -D_LARGEFILE64_SOURCE  -D_FILE_OFFSET_BITS=64 -DBUILD_WITH_MINGW
ifeq ($(strip $(USE_MINGW)),y)
LOCAL_CFLAGS += -DBUILD_DLL 
endif

include $(BUILD_HOST_SHARED_LIBRARY)	
	
	
#################################
	
include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(SRC_FILES)
LOCAL_CFLAGS := -D_LARGEFILE64_SOURCE  -D_FILE_OFFSET_BITS=64 -DBUILD_WITH_MINGW
LOCAL_MODULE := aml_image_packer

include $(BUILD_HOST_EXECUTABLE)

else
##################################
include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(SRC_FILES)
LOCAL_STATIC_LIBRARIES := libc
LOCAL_MODULE := libimagepack
LOCAL_CFLAGS := -DBUILD_LIBRARY -D_LARGEFILE64_SOURCE  -D_FILE_OFFSET_BITS=64

include $(BUILD_STATIC_LIBRARY)
#################################
include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(SRC_FILES)
LOCAL_SYSTEM_SHARED_LIBRARIES := libc
LOCAL_MODULE := libimagepack
LOCAL_CFLAGS := -DBUILD_LIBRARY -D_LARGEFILE64_SOURCE  -D_FILE_OFFSET_BITS=64

include $(BUILD_SHARED_LIBRARY)

	
#################################
	
include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(SRC_FILES)
LOCAL_MODULE := aml_image_packer
LOCAL_CFLAGS := -D_LARGEFILE64_SOURCE  -D_FILE_OFFSET_BITS=64

include $(BUILD_HOST_EXECUTABLE)

endif

