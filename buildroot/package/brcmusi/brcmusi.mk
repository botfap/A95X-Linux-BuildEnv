################################################################################
#
# broadcom usi driver
#
################################################################################

BRCMUSI_VERSION = $(call qstrip,$(BR2_PACKAGE_BRCMUSI_GIT_VERSION))
BRCMUSI_SITE = $(call qstrip,$(BR2_PACKAGE_BRCMUSI_GIT_REPO_URL))
define BRCMUSI_BUILD_CMDS
	mkdir -p $(LINUX_DIR)/../hardware/wifi/broadcom/drivers;
	ln -sf $(BRCMUSI_DIR) $(LINUX_DIR)/../hardware/wifi/broadcom/drivers/usi
endef
$(eval $(generic-package))
