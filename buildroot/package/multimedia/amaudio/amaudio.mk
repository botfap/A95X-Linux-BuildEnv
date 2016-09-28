#############################################################
#
# amaudio
#
#############################################################
AMAUDIO_VERSION:=1.0.0
AMAUDIO_SITE=$(TOPDIR)/package/multimedia/amaudio/src
AMAUDIO_SITE_METHOD=local

define AMAUDIO_BUILD_CMDS
	$(MAKE) CC=$(TARGET_CC) CXX=$(TARGET_CXX) -C $(@D)
endef

define AMAUDIO_CLEAN_CMDS
	$(MAKE) -C $(@D) clean
endef

define AMAUDIO_INSTALL_TARGET_CMDS
	$(MAKE) -C $(@D) install
endef

define AMAUDIO_UNINSTALL_TARGET_CMDS
        $(MAKE) -C $(@D) uninstall
endef

$(eval $(generic-package))
