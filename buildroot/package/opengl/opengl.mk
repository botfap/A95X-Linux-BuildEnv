#############################################################
#
# opengl
#
#############################################################
OPENGL_VERSION:=0.9.9
OPENGL_DIR=$(BUILD_DIR)/opengl
OPENGL_SOURCE=src
OPENGL_SITE=.
OPENGL_VERSION=
ifneq ($(BR2_PACKAGE_OPENGL_VERSION),"")
OPENGL_VERSION=$(BR2_PACKAGE_OPENGL_VERSION)
else
OPENGL_VERSION=$(BR2_PACKAGE_GPU_VERSION)
endif

API_VERSION = $(call qstrip,$(OPENGL_VERSION))
MALI_VERSION = $(call qstrip,$(BR2_PACKAGE_OPENGL_MALI_VERSION))
MALI_LIB_LOC = $(API_VERSION)/$(MALI_VERSION)

ifeq ($(BR2_aarch64),y)
MALI_LIB_DIR=arm64/$(MALI_LIB_LOC)
else ifeq ($(BR2_ARM_EABIHF),y)
MALI_LIB_DIR=eabihf/$(MALI_LIB_LOC)
else
MALI_LIB_DIR=$(MALI_LIB_LOC)
endif

$(OPENGL_DIR)/.unpacked:
	mkdir -p $(OPENGL_DIR)
	cp -arf ./package/opengl/src/* $(OPENGL_DIR)
	cp --remove-destination $(OPENGL_DIR)/lib/$(MALI_LIB_DIR)/*.so* $(OPENGL_DIR)/lib/
	touch $(OPENGL_DIR)/.unpacked

$(OPENGL_DIR)/.installed: $(OPENGL_DIR)/.unpacked
	cp -arf $(OPENGL_DIR)/* $(STAGING_DIR)/usr
	cp -df $(OPENGL_DIR)/lib/*.so* $(TARGET_DIR)/usr/lib
	install -m 755 $(OPENGL_DIR)/lib/$(MALI_LIB_DIR)/*.so* $(TARGET_DIR)/usr/lib
	mkdir -p $(TARGET_DIR)/usr/lib/pkgconfig
	install -m 644 $(OPENGL_DIR)/lib/pkgconfig/*.pc $(TARGET_DIR)/usr/lib/pkgconfig
	touch $(OPENGL_DIR)/.installed

opengl: $(OPENGL_DIR)/.installed

opengl-clean:
	rm -rf $(OPENGL_DIR)

opengl-dirclean:
	rm -rf $(OPENGL_DIR)

#############################################################
#
# Toplevel Makefile options
#
#############################################################
ifeq ($(BR2_PACKAGE_OPENGL),y)
TARGETS+=opengl
endif
include $(sort $(wildcard package/opengl/*/*.mk))
