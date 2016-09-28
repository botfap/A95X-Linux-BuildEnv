################################################################################
#
# amlogic 8189ftv driver
#
################################################################################

RTK8189FTV_VERSION = $(call qstrip,$(BR2_PACKAGE_RTK8189FTV_GIT_VERSION))
RTK8189FTV_SITE = $(call qstrip,$(BR2_PACKAGE_RTK8189FTV_GIT_REPO_URL))
RTK8189FTV_MODULE_DIR = kernel/realtek/wifi
RTK8189FTV_INSTALL_DIR = $(TARGET_DIR)/lib/modules/$(LINUX_VERSION_PROBED)/$(RTK8189FTV_MODULE_DIR)
RTK8189FTV_DEPENDENCIES = linux

ifeq ($(BR2_PACKAGE_RTK8189FTV_STANDALONE),y)
define RTK8189FTV_BUILD_CMDS
	$(TARGET_CONFIGURE_OPTS) $(MAKE) -C $(LINUX_DIR) M=$(@D)/rtl8189FS ARCH=$(KERNEL_ARCH) \
		CROSS_COMPILE=$(TARGET_CROSS) modules
endef
define RTK8189FTV_INSTALL_TARGET_CMDS
	mkdir -p $(RTK8189FTV_INSTALL_DIR)
	$(INSTALL) -m 0666 $(@D)/rtl8189FS/8189fs.ko $(RTK8189FTV_INSTALL_DIR)
	echo $(RTK8189FTV_MODULE_DIR)/8189fs.ko: >> $(TARGET_DIR)/lib/modules/$(LINUX_VERSION_PROBED)/modules.dep
endef
else
define RTK8189FTV_BUILD_CMDS
	$(TARGET_CONFIGURE_OPTS) $(MAKE) -C $(LINUX_DIR) M=$(@D)/rtl8189FTV ARCH=$(KERNEL_ARCH) \
		CROSS_COMPILE=$(TARGET_CROSS) modules
endef
define RTK8189FTV_INSTALL_TARGET_CMDS
	mkdir -p $(RTK8189FTV_INSTALL_DIR)
	$(INSTALL) -m 0666 $(@D)/rtl8189FTV/8189ftv.ko $(RTK8189FTV_INSTALL_DIR)
	echo $(RTK8189FTV_MODULE_DIR)/8189ftv.ko: >> $(TARGET_DIR)/lib/modules/$(LINUX_VERSION_PROBED)/modules.dep
endef
endif
$(eval $(generic-package))
