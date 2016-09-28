#############################################################
#
# mediasupport
#
#############################################################
MEDIASUPPORT_VERSION:=1.0.0
MEDIASUPPORT_SITE=$(TOPDIR)/package/multimedia/mediasupport/src
MEDIASUPPORT_SITE_METHOD=local

define MEDIASUPPORT_INSTALL_YAHOOTV_LIB
	cp $(MEDIASUPPORT_SITE)/*.so $(STAGING_DIR)/usr/lib
endef

MEDIASUPPORT_POST_CONFIGURE_HOOKS += MEDIASUPPORT_INSTALL_YAHOOTV_LIB

define MEDIASUPPORT_BUILD_CMDS
	$(MAKE) CC=$(TARGET_CC) CXX=$(TARGET_CXX) -C $(@D)
endef

define MEDIASUPPORT_CLEAN_CMDS
	$(MAKE) -C $(@D) clean
endef

define MEDIASUPPORT_INSTALL_TARGET_CMDS
	$(MAKE) -C $(@D) install
endef

define MEDIASUPPORT_UNINSTALL_TARGET_CMDS
        $(MAKE) -C $(@D) uninstall
endef

$(eval $(generic-package))
