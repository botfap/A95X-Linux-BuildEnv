################################################################################
#
# amlogic 8188eu driver
#
################################################################################

RTK8188EU_VERSION = $(call qstrip,$(BR2_PACKAGE_RTK8188EU_GIT_VERSION))
RTK8188EU_SITE = $(call qstrip,$(BR2_PACKAGE_RTK8188EU_GIT_REPO_URL))
RTK8188EU_MODULE_DIR = kernel/realtek/wifi
RTK8188EU_INSTALL_DIR = $(TARGET_DIR)/lib/modules/$(LINUX_VERSION_PROBED)/$(RTK8188EU_MODULE_DIR)
ifeq ($(BR2_PACKAGE_RTK8188EU_STANDALONE),y)
RTK8188EU_DEPENDENCIES = linux
endif

ifeq ($(BR2_PACKAGE_RTK8188EU_STANDALONE),y)
define RTK8188EU_BUILD_CMDS
	$(TARGET_CONFIGURE_OPTS) $(MAKE) -C $(LINUX_DIR) M=$(@D)/rtl8xxx_EU ARCH=$(KERNEL_ARCH) \
		CROSS_COMPILE=$(TARGET_CROSS) CONFIG_RTL8188EU=m modules
endef
define RTK8188EU_INSTALL_TARGET_CMDS
	mkdir -p $(RTK8188EU_INSTALL_DIR)
	$(INSTALL) -m 0666 $(@D)/rtl8xxx_EU/8188eu.ko $(RTK8188EU_INSTALL_DIR)
	echo $(RTK8188EU_MODULE_DIR)/8188eu.ko: >> $(TARGET_DIR)/lib/modules/$(LINUX_VERSION_PROBED)/modules.dep
endef
else
define RTK8188EU_BUILD_CMDS
	mkdir -p $(LINUX_DIR)/../hardware/wifi/realtek/drivers;
	ln -sf $(RTK8188EU_DIR) $(LINUX_DIR)/../hardware/wifi/realtek/drivers/8188eu
endef
endif

$(eval $(generic-package))
