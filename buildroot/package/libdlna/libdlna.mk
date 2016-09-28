################################################################################
#
# libdlna
#
################################################################################

LIBDLNA_VERSION = 0.2.4
LIBDLNA_SOURCE = libdlna-$(LIBDLNA_VERSION).tar.bz2
LIBDLNA_SITE = http://libdlna.geexbox.org/releases/
LIBDLNA_DEPENDENCIES = libplayer aml_libs libupnp
LIBDLNA_INSTALL_STAGING = YES
LIBDLNA_LICENSE = GPLv2+
LIBDLNA_LICENSE_FILES = COPYING

define LIBDLNA_CONFIGURE_CMDS
	(cd $(@D); $(TARGET_MAKE_ENV) ./configure  \
		--prefix=$(STAGING_DIR)/usr  \
		--cross-prefix=$(BR2_TOOLCHAIN_EXTERNAL_PREFIX)-  \
		--with-ffmpeg-includedir=$(STAGING_DIR)/usr/include/  \
		--with-ffmpeg-libdir=$(TARGET_DIR)/usr/lib/)
endef

define LIBDLNA_BUILD_CMDS
	(cd $(@D);  $(TARGET_MAKE_ENV) $(MAKE) all)
endef

define LIBDLNA_INSTALL_STAGING_CMDS
	(cd $(@D);  $(TARGET_MAKE_ENV) $(MAKE) install)
endef

define LIBDLNA_INSTALL_TARGET_CMDS
	$(INSTALL) $(@D)/src/libdlna.so.0.2.4 $(TARGET_DIR)/usr/lib
	ln -sf $(TARGET_DIR)/usr/lib/libdlna.so.0.2.4 $(TARGET_DIR)/usr/lib/libdlna.so.0
	ln -sf $(TARGET_DIR)/usr/lib/libdlna.so.0 $(TARGET_DIR)/usr/lib/libdlna.so
endef

$(eval $(generic-package))
