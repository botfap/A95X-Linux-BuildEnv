#############################################################
#
# remotecfg
#
#############################################################
REMOTECFG_VERSION:=1.0.0
REMOTECFG_SITE=$(TOPDIR)/package/remotecfg/src
REMOTECFG_SITE_METHOD=local

define REMOTECFG_BUILD_CMDS
	$(MAKE) CC=$(TARGET_CC) -C $(@D)
endef

define REMOTECFG_CLEAN_CMDS
	$(MAKE) -C $(@D) clean
endef

define REMOTECFG_INSTALL_TARGET_CMDS
	$(MAKE) -C $(@D) install
endef

define REMOTECFG_UNINSTALL_TARGET_CMDS
        $(MAKE) -C $(@D) uninstall
endef

$(eval $(generic-package))
