LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CFLAGS := \
        -fPIC -D_POSIX_SOURCE
ifdef DOLBY_UDC
    LOCAL_CFLAGS+=-DDOLBY_USE_ARMDEC
endif


LOCAL_C_INCLUDES:= \
    $(LOCAL_PATH)/include \
    $(LOCAL_PATH)/../amavutils/include \


ifneq (0, $(shell expr $(PLATFORM_VERSION) \>= 4.3))
    LOCAL_CFLAGS += -DANDROID_VERSION_JBMR2_UP=1  -DUSE_ARM_AUDIO_DEC	
endif

ifneq (0, $(shell expr $(PLATFORM_VERSION) \> 4.1.0))
    LOCAL_CFLAGS += -D_VERSION_JB
else
    ifneq (0, $(shell expr $(PLATFORM_VERSION) \> 4.0.0))
        LOCAL_CFLAGS += -D_VERSION_ICS
    endif
endif

LOCAL_SRC_FILES := \
           adec-external-ctrl.c adec-internal-mgt.c adec-ffmpeg-mgt.c adec-message.c adec-pts-mgt.c feeder.c adec_write.c adec_read.c\
           dsp/audiodsp-ctl.c audio_out/android-out.cpp audio_out/aml_resample.c audiodsp_update_format.c spdif_api.c pcmenc_api.c \
           dts_transenc_api.c dts_enc.c adec_omx_brige.c ../amavutils/amconfigutils.c

LOCAL_MODULE := libamadec

LOCAL_ARM_MODE := arm


include $(BUILD_STATIC_LIBRARY)


include $(CLEAR_VARS)

LOCAL_CFLAGS := \
        -fPIC -D_POSIX_SOURCE
ifdef DOLBY_UDC
    LOCAL_CFLAGS+=-DDOLBY_USE_ARMDEC
endif


LOCAL_C_INCLUDES:= \
    $(LOCAL_PATH)/include \
    $(LOCAL_PATH)/../amavutils/include \

ifneq (0, $(shell expr $(PLATFORM_VERSION) \>= 4.3))
    LOCAL_CFLAGS += -DANDROID_VERSION_JBMR2_UP=1
endif


ifneq (0, $(shell expr $(PLATFORM_VERSION) \> 4.1.0))
    LOCAL_CFLAGS += -D_VERSION_JB
else
    ifneq (0, $(shell expr $(PLATFORM_VERSION) \> 4.0.0))
        LOCAL_CFLAGS += -D_VERSION_ICS
    endif
endif

LOCAL_SRC_FILES := \
           adec-external-ctrl.c adec-internal-mgt.c adec-ffmpeg-mgt.c adec-message.c adec-pts-mgt.c feeder.c adec_write.c adec_read.c\
           dsp/audiodsp-ctl.c audio_out/android-out.cpp audio_out/aml_resample.c audiodsp_update_format.c \
           spdif_api.c pcmenc_api.c dts_transenc_api.c dts_enc.c adec_omx_brige.c ../amavutils/amconfigutils.c

LOCAL_MODULE := libamadec

LOCAL_ARM_MODE := arm
##################################################
#$(shell cp $(LOCAL_PATH)/acodec_lib/*.so $(TARGET_OUT)/lib)
###################################################
LOCAL_SHARED_LIBRARIES += libutils libmedia libz libbinder libdl libcutils libc libamadec_omx_api libamavutils

LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_TAGS := optional
include $(BUILD_SHARED_LIBRARY)


###################module make file for libamadec_omx_api ######################################
include $(CLEAR_VARS)

LOCAL_CFLAGS := \
        -fPIC -D_POSIX_SOURCE  -DDOLBY_DDPDEC51_MULTICHANNEL_ENDPOINT

LOCAL_C_INCLUDES:= \
    $(LOCAL_PATH)/omx_audio/include \
    $(LOCAL_PATH)/omx_audio/../     \
    $(LOCAL_PATH)/omx_audio/../include \
    $(LOCAL_PATH)/omx_audio/../../../../../frameworks/native/include/media/openmax \
    $(LOCAL_PATH)/omx_audio/../../../../../frameworks/av/include/media/stagefright \
    $(LOCAL_PATH)/omx_audio/../../../../../frameworks/native/include/utils

LOCAL_SRC_FILES := \
           /omx_audio/adec_omx.cpp \
           /omx_audio/audio_mediasource.cpp \
           /omx_audio/DDP_mediasource.cpp \
           /omx_audio/ALAC_mediasource.cpp \
           /omx_audio/MP3_mediasource.cpp \
	   /omx_audio/ASF_mediasource.cpp  \
	   /omx_audio/DTSHD_mediasource.cpp \
	   /omx_audio/Vorbis_mediasource.cpp \

LOCAL_MODULE := libamadec_omx_api
LOCAL_MODULE_TAGS := optional
LOCAL_ARM_MODE := arm

LOCAL_SHARED_LIBRARIES += libutils libmedia libz libbinder libdl libcutils libc libstagefright \
                          libstagefright_omx  libstagefright_yuv libmedia_native liblog 
LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)
#########################################################


#
# audio_firmware module
#   includes all audio firmware files, which are modules themselves.
#

include $(CLEAR_VARS)

ifeq ($(TARGET_BOARD_PLATFORM),meson6)
    audio_firmware_dir := firmware-m6
else ifeq ($(TARGET_BOARD_PLATFORM),meson8)
    audio_firmware_dir := firmware-m8
else
    audio_firmware_dir := firmware
endif

# generate md5 checksum files
$(shell cd $(LOCAL_PATH)/$(audio_firmware_dir) && { \
for f in *.bin; do \
  md5sum "$$f" > "$$f".checksum; \
done;})

# gather list of relative filenames
audio_firmware_files := $(wildcard $(LOCAL_PATH)/$(audio_firmware_dir)/*.bin)
audio_firmware_files += $(wildcard $(LOCAL_PATH)/$(audio_firmware_dir)/*.checksum)
audio_firmware_files := $(patsubst $(LOCAL_PATH)/%,%,$(audio_firmware_files))

# define function to create a module for each file
# $(1): filename
define _add-audio-firmware-module
    include $$(CLEAR_VARS)
    LOCAL_MODULE := audio-firmware_$(notdir $(1))
    LOCAL_MODULE_STEM := $(notdir $(1))
    _audio_firmware_modules += $$(LOCAL_MODULE)
    LOCAL_SRC_FILES := $1
    LOCAL_MODULE_TAGS := optional
    LOCAL_MODULE_CLASS := ETC
    LOCAL_MODULE_PATH := $$(TARGET_OUT_ETC)/firmware
    include $$(BUILD_PREBUILT)
endef

# create modules, one for each file
_audio_firmware_modules :=
_audio_firmware :=
$(foreach _firmware, $(audio_firmware_files), \
  $(eval $(call _add-audio-firmware-module,$(_firmware))))

include $(CLEAR_VARS)

LOCAL_MODULE := audio_firmware
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_TAGS := optional

LOCAL_REQUIRED_MODULES := $(_audio_firmware_modules)

include $(BUILD_PHONY_PACKAGE)

_audio_firmware_modules :=
_audio_firmware :=

include $(call all-subdir-makefiles)
