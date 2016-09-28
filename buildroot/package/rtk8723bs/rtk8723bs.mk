################################################################################
#
# amlogic 8723bs driver
#
################################################################################

RTK8723BS_VERSION = $(call qstrip,$(BR2_PACKAGE_RTK8723BS_GIT_VERSION))
RTK8723BS_SITE = $(call qstrip,$(BR2_PACKAGE_RTK8723BS_GIT_REPO_URL))
RTK8723BS_MODULE_DIR = kernel/realtek/wifi
RTK8723BS_INSTALL_DIR = $(TARGET_DIR)/lib/modules/$(LINUX_VERSION_PROBED)/$(RTK8723BS_MODULE_DIR)

ifeq ($(BR2_PACKAGE_RTK8723BS_STANDALONE),y)
define RTK8723BS_BUILD_CMDS
	$(TARGET_CONFIGURE_OPTS) $(MAKE) -C $(LINUX_DIR) M=$(@D)/rtl8723BS ARCH=$(KERNEL_ARCH) \
		CROSS_COMPILE=$(TARGET_CROSS) modules
endef
define RTK8723BS_INSTALL_TARGET_CMDS
	mkdir -p $(RTK8723BS_INSTALL_DIR)
	$(INSTALL) -m 0666 $(@D)/rtl8723BS/8723bs.ko $(RTK8723BS_INSTALL_DIR)
	echo $(RTK8723BS_MODULE_DIR)/8723bs.ko: >> $(TARGET_DIR)/lib/modules/$(LINUX_VERSION_PROBED)/modules.dep
endef
else
define RTK8723BS_BUILD_CMDS
	mkdir -p $(LINUX_DIR)/../hardware/wifi/realtek/drivers;
	ln -sf $(RTK8723BS_DIR) $(LINUX_DIR)/../hardware/wifi/realtek/drivers/8723bs
endef
endif
$(eval $(generic-package))
