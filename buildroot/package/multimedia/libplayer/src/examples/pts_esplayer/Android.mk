LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE    := ESPlayer
LOCAL_MODULE_TAGS := ESPlayer

LOCAL_SRC_FILES := ESPlayer.c \
									main.c
									
LOCAL_ARM_MODE := arm
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../../amplayer/player/include \
    $(LOCAL_PATH)/../../amcodec/include \
    $(LOCAL_PATH)/../../amadec/include \
    $(LOCAL_PATH)/../../amffmpeg \
    $(JNI_H_INCLUDE) 

LOCAL_SHARED_LIBRARIES := libamplayer libamplayer libamcodec libavformat libavcodec libavutil libamadec libamavutils 
LOCAL_SHARED_LIBRARIES += libutils libmedia libbinder libz libdl libcutils libssl libcrypto 

include $(BUILD_EXECUTABLE)


