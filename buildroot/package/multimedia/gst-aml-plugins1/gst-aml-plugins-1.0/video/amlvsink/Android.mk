LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

include $(LOCAL_PATH)/../../../../build/android/base.mak

LOCAL_MODULE_TAGS := optional

LOCAL_C_INCLUDES += \
	$(gcv_3rd_src_prefix)/gstreamer-1.2.3 \
    $(gcv_3rd_install_prefix)/include/gstreamer-1.0 \
    $(gcv_3rd_install_prefix)/include/gstreamer-1.0/gst \
    $(gcv_3rd_install_prefix)/include/gstreamer-1.0/gst/app \
    $(gcv_3rd_install_prefix)/include/glib-2.0 \
    $(gcv_3rd_install_prefix)/include/glib-2.0/glib \
    $(gcv_3rd_install_prefix)/include/glib-2.0/gio \
    $(gcv_3rd_install_prefix)/lib/glib-2.0/include \
    $(gcv_3rd_install_prefix)/include/glib-2.0/glib/gobject


LOCAL_SRC_FILES := gstamlvsink.c 

#LOCAL_STATIC_LIBRARIES +=
LOCAL_SHARED_LIBRARIES += libamlstreaminfo

LOCAL_LDFLAGS += -L$(gcv_sme_out_so_path) -L$(gcv_3rd_install_prefix)/lib \
	-lgstreamer-1.0 -lgstvideo-1.0 -lglib-2.0 -lgobject-2.0  

LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/gstplugins
LOCAL_MODULE := libsme_amlvsink

include $(BUILD_SHARED_LIBRARY)
