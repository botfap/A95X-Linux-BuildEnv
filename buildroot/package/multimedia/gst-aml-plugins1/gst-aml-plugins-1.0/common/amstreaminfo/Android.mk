#android makefile for libsme_player.a
LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

include $(LOCAL_PATH)/../../../../build/android/base.mak

LOCAL_MODULE_TAGS := optional

AMPLAYER_APK_DIR=vendor/amlogic/frameworks/av/LibPlayer

LOCAL_C_INCLUDES += \
	$(gcv_3rd_src_prefix)/gstreamer-1.2.3 \
    $(gcv_3rd_install_prefix)/include/gstreamer-1.0 \
    $(gcv_3rd_install_prefix)/include/gstreamer-1.0/gst \
    $(gcv_3rd_install_prefix)/include/gstreamer-1.0/gst/app \
    $(gcv_3rd_install_prefix)/include/glib-2.0 \
    $(gcv_3rd_install_prefix)/include/glib-2.0/glib \
    $(gcv_3rd_install_prefix)/include/glib-2.0/gio \
    $(gcv_3rd_install_prefix)/lib/glib-2.0/include \
    $(gcv_3rd_install_prefix)/include/glib-2.0/glib/gobject \
    $(AMPLAYER_APK_DIR)/amplayer/player/include     \
	$(AMPLAYER_APK_DIR)/amplayer/control/include    \
	$(AMPLAYER_APK_DIR)/amadec/include              \
	$(AMPLAYER_APK_DIR)/amcodec/include             \
	$(AMPLAYER_APK_DIR)/amavutils/include           \
	$(AMPLAYER_APK_DIR)/amvdec/include           \
	$(AMPLAYER_APK_DIR)/amffmpeg/
	
        
LOCAL_SRC_FILES := amlstreaminfo.c amlutils.c

#LOCAL_STATIC_LIBRARIES +=
#LOCAL_SHARED_LIBRARIES += libsme_generic libsme_mediautils

LOCAL_LDFLAGS += -L$(gcv_sme_out_so_path) -L$(gcv_3rd_install_prefix)/lib \
	-lgstreamer-1.0 -lglib-2.0 -lamplayer
#	-lgstreamer-1.0 -lglib-2.0 -lgobject-2.0 -lamplayer
#	-lsme_generic -lsme_mediautils \
#	-lgstreamer-1.0 -lgstbase-1.0 -lglib-2.0 -lgobject-2.0 \
#	-lgmodule-2.0 -lgio-2.0 -lgstvideo-1.0 -lgstapp-1.0

#LOCAL_MODULE_PATH := $(LOCAL_PATH)/../../build/android/out/lib

LOCAL_MODULE := libamlstreaminfo

include $(BUILD_SHARED_LIBRARY)
