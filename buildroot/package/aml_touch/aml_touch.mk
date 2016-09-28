################################################################################
#
# amlogic touch driver
#
################################################################################

AML_TOUCH_VERSION = $(call qstrip,$(BR2_PACKAGE_AML_TOUCH_GIT_VERSION))
AML_TOUCH_SITE = $(call qstrip,$(BR2_PACKAGE_AML_TOUCH_GIT_REPO_URL))
define AML_TOUCH_BUILD_CMDS
	mkdir -p $(LINUX_DIR)/../hardware/amlogic;
	ln -sf $(AML_TOUCH_DIR) $(LINUX_DIR)/../hardware/amlogic/touch
endef
$(eval $(generic-package))
