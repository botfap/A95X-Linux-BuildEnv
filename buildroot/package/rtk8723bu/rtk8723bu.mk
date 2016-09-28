################################################################################
#
# amlogic 8723bu driver
#
################################################################################

RTK8723BU_VERSION = $(call qstrip,$(BR2_PACKAGE_RTK8723BU_GIT_VERSION))
RTK8723BU_SITE = $(call qstrip,$(BR2_PACKAGE_RTK8723BU_GIT_REPO_URL))
RTK8723BU_MODULE_DIR = kernel/realtek/wifi
RTK8723BU_INSTALL_DIR = $(TARGET_DIR)/lib/modules/$(LINUX_VERSION_PROBED)/$(RTK8723BU_MODULE_DIR)

ifeq ($(BR2_PACKAGE_RTK8723BU_STANDALONE),y)
define RTK8723BU_BUILD_CMDS
	$(TARGET_CONFIGURE_OPTS) $(MAKE1) -C $(LINUX_DIR) M=$(@D)/rtl8723BU ARCH=$(KERNEL_ARCH) \
		CONFIG_RTL8723BU=m CROSS_COMPILE=$(TARGET_CROSS) modules
endef
define RTK8723BU_INSTALL_TARGET_CMDS
	mkdir -p $(RTK8723BU_INSTALL_DIR)
	$(INSTALL) -m 0666 $(@D)/rtl8723BU/8723bu.ko $(RTK8723BU_INSTALL_DIR)
	echo $(RTK8723BU_MODULE_DIR)/8723bu.ko: >> $(TARGET_DIR)/lib/modules/$(LINUX_VERSION_PROBED)/modules.dep
endef
else
define RTK8723BU_BUILD_CMDS
	mkdir -p $(LINUX_DIR)/../hardware/wifi/realtek/drivers;
	ln -sf $(RTK8723BU_DIR) $(LINUX_DIR)/../hardware/wifi/realtek/drivers/8723bu
endef
endif

$(eval $(generic-package))
