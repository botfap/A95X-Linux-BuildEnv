################################################################################
#
# shairport
#
################################################################################

SHAIRPORT_VERSION = d679d19a4bd66cc220dabfd23ad748c34e95995c
SHAIRPORT_SITE = git://github.com/abrasive/shairport.git
SHAIRPORT_LICENSE = GPLv2 LGPLv2.1
SHAIRPORT_LICENSE_FILES = LICENSE
SHAIRPORT_INSTALL_STAGING = YES
SHAIRPORT_DEPENDENCIES = alsa-lib openssl avahi
SHAIRPORT_CFLAGS = $(TARGET_CFLAGS) -mhard-float
SHAIRPORT_CONF_OPTS = \
		--enable-alsa \
		--with-openssl \
		--enable-avahi \
		-Dccflags="$(SHAIRPORT_CFLAGS)"

SHAIRPORT_MAKE_ENV = \
	CC="$(TARGET_CC)" \
	CFLAGS="$(SHAIRPORT_CFLAGS)" \
	$(TARGET_MAKE_ENV)

define SHAIRPORT_CONFIGURE_CMDS
	cd $(@D); \
	$(TARGET_CONFIGURE_ARGS) \
	$(TARGET_CONFIGURE_OPTS) \
	./configure
endef

define SHAIRPORT_BUILD_CMDS
	$(SHAIRPORT_MAKE_ENV) $(MAKE) -C $(@D)
endef

define SHAIRPORT_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/shairport $(TARGET_DIR)/usr/bin/shairport
endef

define SHAIRPORT_CLEAN_CMDS
	$(MAKE) -C $(@D) DESTDIR=$(STAGING_DIR) uninstall
	$(MAKE) -C $(@D) clean
	rm -f $(TARGET_DIR)/usr/bin/shairport
endef

$(eval $(autotools-package))
