#by peter
LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_TAGS := tests
LOCAL_ARM_MODE := arm
LOCAL_SRC_FILES := parser_testcase.c 

LOCAL_C_INCLUDES := \
        $(LOCAL_PATH)/../common\
        $(LOCAL_PATH)/../hls_main\
        $(LOCAL_PATH)/../downloader\
        $(LOCAL_PATH)/../include
        

LOCAL_MODULE := m3uparser_test 
LOCAL_STATIC_LIBRARIES := libhls libhls_http libhls_common 

LOCAL_SHARED_LIBRARIES :=libamplayer libcutils libssl libamavutils libcrypto
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_TAGS := tests
LOCAL_ARM_MODE := arm
LOCAL_SRC_FILES := hls_merge_tool.c 

LOCAL_C_INCLUDES := \
        $(LOCAL_PATH)/../common\
        $(LOCAL_PATH)/../include
        

LOCAL_MODULE := hls_merge_tool 
LOCAL_STATIC_LIBRARIES := libhls libhls_http libhls_common 

LOCAL_SHARED_LIBRARIES :=libamplayer libcutils libssl libamavutils libcrypto
include $(BUILD_EXECUTABLE)

