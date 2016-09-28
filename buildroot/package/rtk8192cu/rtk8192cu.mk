################################################################################
#
# amlogic 8192cu driver
#
################################################################################

RTK8192CU_VERSION = $(call qstrip,$(BR2_PACKAGE_RTK8192CU_GIT_VERSION))
RTK8192CU_SITE = $(call qstrip,$(BR2_PACKAGE_RTK8192CU_GIT_REPO_URL))
RTK8192CU_MODULE_DIR = kernel/realtek/wifi
RTK8192CU_INSTALL_DIR = $(TARGET_DIR)/lib/modules/$(LINUX_VERSION_PROBED)/$(RTK8192CU_MODULE_DIR)

ifeq ($(BR2_PACKAGE_RTK8192CU_STANDALONE),y)
RTK8192CU_DEPENDENCIES = linux
endif
ifeq ($(BR2_PACKAGE_RTK8192CU_STANDALONE),y)
define RTK8192CU_BUILD_CMDS
	$(TARGET_CONFIGURE_OPTS) $(MAKE) -C $(LINUX_DIR) M=$(@D)/rtl8xxx_CU ARCH=$(KERNEL_ARCH) \
		CROSS_COMPILE=$(TARGET_CROSS) CONFIG_RTL8192CU=m modules
endef
define RTK8192CU_INSTALL_TARGET_CMDS
	mkdir -p $(RTK8192CU_INSTALL_DIR)
	$(INSTALL) -m 0666 $(@D)/rtl8xxx_CU/8192cu.ko $(RTK8192CU_INSTALL_DIR)
	echo $(RTK8192CU_MODULE_DIR)/8192cu.ko: >> $(TARGET_DIR)/lib/modules/$(LINUX_VERSION_PROBED)/modules.dep
endef
else
define RTK8192CU_BUILD_CMDS
	mkdir -p $(LINUX_DIR)/../hardware/wifi/realtek/drivers;
	ln -sf $(RTK8192CU_DIR) $(LINUX_DIR)/../hardware/wifi/realtek/drivers/8192cu
endef
endif

$(eval $(generic-package))
