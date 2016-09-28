#############################################################
#
# aml utility
#
#############################################################
AML_UTIL_VERSION = 0.1
AML_UTIL_SITE = $(TOPDIR)/package/aml_util/src
AML_UTIL_SITE_METHOD = local

define AML_UTIL_BUILD_CMDS
	$(TARGET_CONFIGURE_OPTS) $(MAKE) -C $(@D) all
endef

define AML_UTIL_INSTALL_TARGET_CMDS
	$(TARGET_CONFIGURE_OPTS) $(MAKE) CC=$(TARGET_CC) -C $(@D) install
endef

$(eval $(generic-package))
