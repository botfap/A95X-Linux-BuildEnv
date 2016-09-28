################################################################################
#
# amlogic 8812au driver
#
################################################################################

RTK8812AU_VERSION = $(call qstrip,$(BR2_PACKAGE_RTK8812AU_GIT_VERSION))
RTK8812AU_SITE = $(call qstrip,$(BR2_PACKAGE_RTK8812AU_GIT_REPO_URL))
RTK8812AU_MODULE_DIR = kernel/realtek/wifi
RTK8812AU_INSTALL_DIR = $(TARGET_DIR)/lib/modules/$(LINUX_VERSION_PROBED)/$(RTK8812AU_MODULE_DIR)
ifeq ($(BR2_PACKAGE_RTK8812AU_STANDALONE),y)
RTK8812AU_DEPENDENCIES = linux
endif

ifeq ($(BR2_PACKAGE_RTK8812AU_STANDALONE),y)
define RTK8812AU_BUILD_CMDS
	$(TARGET_CONFIGURE_OPTS) $(MAKE) -C $(LINUX_DIR) M=$(@D)/rtl8812AU ARCH=$(KERNEL_ARCH) \
		CROSS_COMPILE=$(TARGET_CROSS) modules
endef
define RTK8812AU_INSTALL_TARGET_CMDS
	mkdir -p $(RTK8812AU_INSTALL_DIR)
	$(INSTALL) -m 0666 $(@D)/rtl8812AU/8812au.ko $(RTK8812AU_INSTALL_DIR)
	echo $(RTK8812AU_MODULE_DIR)/8812au.ko: >> $(TARGET_DIR)/lib/modules/$(LINUX_VERSION_PROBED)/modules.dep
endef
else

define RTK8812AU_BUILD_CMDS
	mkdir -p $(LINUX_DIR)/../hardware/wifi/realtek/drivers;
	ln -sf $(RTK8812AU_DIR) $(LINUX_DIR)/../hardware/wifi/realtek/drivers/8812au
endef
endif
$(eval $(generic-package))
