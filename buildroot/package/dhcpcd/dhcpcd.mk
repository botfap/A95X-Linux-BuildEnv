################################################################################
#
# dhcpcd
#
################################################################################

DHCPCD_VERSION = 6.10.1
DHCPCD_SOURCE = dhcpcd-$(DHCPCD_VERSION).tar.xz
DHCPCD_SITE = http://roy.marples.name/downloads/dhcpcd
DHCPCD_DEPENDENCIES = host-pkgconf
DHCPCD_LICENSE = BSD-2c
DHCPCD_LICENSE_FILES = dhcpcd.c

ifeq ($(BR2_STATIC_LIBS),y)
DHCPCD_CONFIG_OPTS += --enable-static
endif

ifeq ($(BR2_USE_MMU),)
DHCPCD_CONFIG_OPTS += --disable-fork
endif

define DHCPCD_CONFIGURE_CMDS
	(cd $(@D); \
	$(TARGET_CONFIGURE_OPTS) ./configure \
		--os=linux \
		--prefix=/ \
		--libexecdir=/lib/dhcpcd \
		$(DHCPCD_CONFIG_OPT))
endef

define DHCPCD_BUILD_CMDS
	$(TARGET_MAKE_ENV) $(MAKE) \
		-C $(@D) all
	$(TARGET_MAKE_ENV) $(MAKE) \
		-C $(@D)/dhcpcd-hooks 
endef

define DHCPCD_INSTALL_TARGET_CMDS
	$(TARGET_MAKE_ENV) $(MAKE) -C $(@D) install DESTDIR=$(TARGET_DIR)
	$(INSTALL) -D -m 0755 $(@D)/dhcpcd \
		$(TARGET_DIR)/usr/sbin/dhcpcd
	$(INSTALL) -D -m 0644 $(@D)/dhcpcd.conf \
		$(TARGET_DIR)/etc/dhcpcd.conf
	$(INSTALL) -D -m 0755 $(@D)/dhcpcd-run-hooks \
		$(TARGET_DIR)/libexec/dhcpcd-run-hooks
	mkdir -p $(TARGET_DIR)/libexec/dhcpcd-hooks/
	$(INSTALL) -D -m 0644 $(@D)/dhcpcd-hooks/01-test \
		$(TARGET_DIR)/libexec/dhcpcd-hooks/
	$(INSTALL) -D -m 0644 $(@D)/dhcpcd-hooks/02-dump \
		$(TARGET_DIR)/libexec/dhcpcd-hooks/
	$(INSTALL) -D -m 0644 $(@D)/dhcpcd-hooks/10-wpa_supplicant \
		$(TARGET_DIR)/libexec/dhcpcd-hooks/
	$(INSTALL) -D -m 0644 $(@D)/dhcpcd-hooks/15-timezone \
		$(TARGET_DIR)/libexec/dhcpcd-hooks/
	$(INSTALL) -D -m 0644 $(@D)/dhcpcd-hooks/20-resolv.conf \
		$(TARGET_DIR)/libexec/dhcpcd-hooks/
	$(INSTALL) -D -m 0644 $(@D)/dhcpcd-hooks/29-lookup-hostname \
		$(TARGET_DIR)/libexec/dhcpcd-hooks/
	$(INSTALL) -D -m 0644 $(@D)/dhcpcd-hooks/30-hostname \
		$(TARGET_DIR)/libexec/dhcpcd-hooks/
	$(INSTALL) -D -m 0644 $(@D)/dhcpcd-hooks/50-dhcpcd-compat \
		$(TARGET_DIR)/libexec/dhcpcd-hooks/
	$(INSTALL) -D -m 0644 $(@D)/dhcpcd-hooks/50-ntp.conf \
		$(TARGET_DIR)/libexec/dhcpcd-hooks/
	$(INSTALL) -D -m 0644 $(@D)/dhcpcd-hooks/50-ypbind \
		$(TARGET_DIR)/libexec/dhcpcd-hooks/
	$(INSTALL) -D -m 0644 $(@D)/dhcpcd-hooks/50-yp.conf \
		$(TARGET_DIR)/libexec/dhcpcd-hooks/
endef

# NOTE: Even though this package has a configure script, it is not generated
# using the autotools, so we have to use the generic package infrastructure.

$(eval $(generic-package))
