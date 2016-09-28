LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := dvb/dvb.c \
                   dvb_chmgr/am_chmgr.c \
                   dvb_chmgr/chmgr_file.c \
                   dvb_dmx/am_dmx.c \
                   dvb_dmx/linux_dvb/linux_dvb.c \
                   dvb_fe/am_fend.c \
                   dvb_fe/linux_dvb/linux_dvb.c \
                   dvb_misc/am_evt.c \
                   dvb_misc/am_misc.c \
                   dvb_parser/dvb_parser.c \
                   dvb_play/am_av.c \
                   dvb_play/aml/aml.c \

LOCAL_C_INCLUDES := \
  $(LOCAL_PATH)/../amplayer/player/include \
  $(LOCAL_PATH)/include \
  $(LOCAL_PATH)/../amffmpeg \
  $(LOCAL_PATH)/../amcodec/include \
  $(LOCAL_PATH)/../amadec/include \
  
LOCAL_ARM_MODE := arm
LOCAL_MODULE := libamdvb

include $(BUILD_STATIC_LIBRARY)

include $(call all-subdir-makefiles)
