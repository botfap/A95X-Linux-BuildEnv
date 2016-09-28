#############################################################
#
# audiocapture
#
#############################################################
AUDIOCAPTURE_VERSION:=1.0.0
AUDIOCAPTURE_SITE=$(TOPDIR)/package/multimedia/audiocapture/src
AUDIOCAPTURE_SITE_METHOD=local

define AUDIOCAPTURE_INSTALL_YAHOOTV_LIB
        cp $(AUDIOCAPTURE_SITE)/*.so $(STAGING_DIR)/usr/lib
endef

AUDIOCAPTURE_POST_CONFIGURE_HOOKS += AUDIOCAPTURE_INSTALL_YAHOOTV_LIB

define AUDIOCAPTURE_BUILD_CMDS
	$(MAKE) CC=$(TARGET_CC) CXX=$(TARGET_CXX) -C $(@D)
endef

define AUDIOCAPTURE_CLEAN_CMDS
	$(MAKE) -C $(@D) clean
endef

define AUDIOCAPTURE_INSTALL_TARGET_CMDS
	$(MAKE) -C $(@D) install
endef

define AUDIOCAPTURE_UNINSTALL_TARGET_CMDS
        $(MAKE) -C $(@D) uninstall
endef

$(eval $(generic-package))
