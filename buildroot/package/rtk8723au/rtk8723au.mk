################################################################################
#
# amlogic 8723au driver
#
################################################################################

RTK8723AU_VERSION = $(call qstrip,$(BR2_PACKAGE_RTK8723AU_GIT_VERSION))
RTK8723AU_SITE = $(call qstrip,$(BR2_PACKAGE_RTK8723AU_GIT_REPO_URL))
define RTK8723AU_BUILD_CMDS
	mkdir -p $(LINUX_DIR)/../hardware/wifi/realtek/drivers;
	ln -sf $(RTK8723AU_DIR) $(LINUX_DIR)/../hardware/wifi/realtek/drivers/8723au
endef
$(eval $(generic-package))
