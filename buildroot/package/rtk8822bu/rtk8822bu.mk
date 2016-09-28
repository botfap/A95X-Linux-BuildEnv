################################################################################
#
# amlogic 8822bu driver
#
################################################################################

RTK8822BU_VERSION = $(call qstrip,$(BR2_PACKAGE_RTK8822BU_GIT_VERSION))
RTK8822BU_SITE = $(call qstrip,$(BR2_PACKAGE_RTK8822BU_GIT_REPO_URL))
RTK8822BU_MODULE_DIR = kernel/realtek/wifi
RTK8822BU_INSTALL_DIR = $(TARGET_DIR)/lib/modules/$(LINUX_VERSION_PROBED)/$(RTK8822BU_MODULE_DIR)
ifeq ($(BR2_PACKAGE_RTK8822BU_STANDALONE),y)
RTK8822BU_DEPENDENCIES = linux
endif

ifeq ($(BR2_PACKAGE_RTK8822BU_STANDALONE),y)
define RTK8822BU_BUILD_CMDS
	$(TARGET_CONFIGURE_OPTS) $(MAKE) -C $(LINUX_DIR) M=$(@D)/rtl8822BU ARCH=$(KERNEL_ARCH) \
		CROSS_COMPILE=$(TARGET_CROSS) modules
endef
define RTK8822BU_INSTALL_TARGET_CMDS
	mkdir -p $(RTK8822BU_INSTALL_DIR)
	$(INSTALL) -m 0666 $(@D)/rtl8822BU/8822bu.ko $(RTK8822BU_INSTALL_DIR)
	echo $(RTK8822BU_MODULE_DIR)/8822bu.ko: >> $(TARGET_DIR)/lib/modules/$(LINUX_VERSION_PROBED)/modules.dep
endef
else

define RTK8822BU_BUILD_CMDS
	mkdir -p $(LINUX_DIR)/../hardware/wifi/realtek/drivers;
	ln -sf $(RTK8822BU_DIR) $(LINUX_DIR)/../hardware/wifi/realtek/drivers/8822bu
endef
endif
$(eval $(generic-package))
