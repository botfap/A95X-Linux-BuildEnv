################################################################################
#
# amlogic 8811au driver
#
################################################################################

RTK8811AU_VERSION = $(call qstrip,$(BR2_PACKAGE_RTK8811AU_GIT_VERSION))
RTK8811AU_SITE = $(call qstrip,$(BR2_PACKAGE_RTK8811AU_GIT_REPO_URL))
define RTK8811AU_BUILD_CMDS
	mkdir -p $(LINUX_DIR)/../hardware/wifi/realtek/drivers;
	ln -sf $(RTK8811AU_DIR) $(LINUX_DIR)/../hardware/wifi/realtek/drivers/8811au
endef
$(eval $(generic-package))
