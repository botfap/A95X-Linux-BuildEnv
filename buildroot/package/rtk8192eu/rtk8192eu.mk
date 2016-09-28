################################################################################
#
# amlogic 8192eu driver
#
################################################################################

RTK8192EU_VERSION = $(call qstrip,$(BR2_PACKAGE_RTK8192EU_GIT_VERSION))
RTK8192EU_SITE = $(call qstrip,$(BR2_PACKAGE_RTK8192EU_GIT_REPO_URL))
RTK8192EU_MODULE_DIR = kernel/realtek/wifi
RTK8192EU_INSTALL_DIR = $(TARGET_DIR)/lib/modules/$(LINUX_VERSION_PROBED)/$(RTK8192EU_MODULE_DIR)

ifeq ($(BR2_PACKAGE_RTK8192EU_STANDALONE),y)
RTK8192EU_DEPENDENCIES = linux
endif
ifeq ($(BR2_PACKAGE_RTK8192EU_STANDALONE),y)
define RTK8192EU_BUILD_CMDS
	$(TARGET_CONFIGURE_OPTS) $(MAKE) -C $(LINUX_DIR) M=$(@D)/rtl8192EU ARCH=$(KERNEL_ARCH) \
		CROSS_COMPILE=$(TARGET_CROSS) CONFIG_RTL8192EU=m modules
endef
define RTK8192EU_INSTALL_TARGET_CMDS
	mkdir -p $(RTK8192EU_INSTALL_DIR)
	$(INSTALL) -m 0666 $(@D)/rtl8192EU/8192eu.ko $(RTK8192EU_INSTALL_DIR)
	echo $(RTK8192EU_MODULE_DIR)/8192eu.ko: >> $(TARGET_DIR)/lib/modules/$(LINUX_VERSION_PROBED)/modules.dep
endef
else

define RTK8192EU_BUILD_CMDS
	mkdir -p $(LINUX_DIR)/../hardware/wifi/realtek/drivers;
	ln -sf $(RTK8192EU_DIR) $(LINUX_DIR)/../hardware/wifi/realtek/drivers/8192eu
endef
endif

$(eval $(generic-package))
