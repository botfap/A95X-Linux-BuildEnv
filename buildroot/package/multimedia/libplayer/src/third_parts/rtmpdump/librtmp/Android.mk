LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_ARM_MODE := arm

LOCAL_PRELINK_MODULE := false

LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := $(notdir $(wildcard $(LOCAL_PATH)/*.c))

OPENSSL_HEADER_DIR=$(TOP)/external/openssl/include
ZLIB_HEADER_DIR=$(TOP)/external/zlib
#LOCAL_CFLAGS+=
LOCAL_C_INCLUDES += \
	${OPENSSL_HEADER_DIR}\
	${ZLIB_HEADER_DIR}

LOCAL_SHARED_LIBRARIES +=liblog libcutils libssl libcrypto libz libdl 

LOCAL_MODULE :=librtmp

include $(BUILD_SHARED_LIBRARY)
#static library
include $(CLEAR_VARS)

LOCAL_ARM_MODE := arm

LOCAL_PRELINK_MODULE := false

LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := $(notdir $(wildcard $(LOCAL_PATH)/*.c))

OPENSSL_HEADER_DIR=$(TOP)/external/openssl/include
ZLIB_HEADER_DIR=$(TOP)/external/zlib
#LOCAL_CFLAGS+=
LOCAL_C_INCLUDES += \
        ${OPENSSL_HEADER_DIR}\
        ${ZLIB_HEADER_DIR}

LOCAL_SHARED_LIBRARIES +=libssl libcrypto libz libdl

LOCAL_MODULE :=librtmp

include $(BUILD_STATIC_LIBRARY)
