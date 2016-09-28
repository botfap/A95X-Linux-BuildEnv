################################################################################
#
# amlogic 8189es driver
#
################################################################################

RTK8189ES_VERSION = $(call qstrip,$(BR2_PACKAGE_RTK8189ES_GIT_VERSION))
RTK8189ES_SITE = $(call qstrip,$(BR2_PACKAGE_RTK8189ES_GIT_REPO_URL))
RTK8189ES_MODULE_DIR = kernel/realtek/wifi
RTK8189ES_INSTALL_DIR = $(TARGET_DIR)/lib/modules/$(LINUX_VERSION_PROBED)/$(RTK8189ES_MODULE_DIR)
ifeq ($(BR2_PACKAGE_RTK8189ES_STANDALONE),y)
RTK8189ES_DEPENDENCIES = linux
endif

ifeq ($(BR2_PACKAGE_RTK8189ES_STANDALONE),y)
define RTK8189ES_BUILD_CMDS
	$(TARGET_CONFIGURE_OPTS) $(MAKE) -C $(LINUX_DIR) M=$(@D)/rtl8189ES ARCH=$(KERNEL_ARCH) \
		CROSS_COMPILE=$(TARGET_CROSS) modules
endef
define RTK8189ES_INSTALL_TARGET_CMDS
	mkdir -p $(RTK8189ES_INSTALL_DIR)
	$(INSTALL) -m 0666 $(@D)/rtl8189ES/8189es.ko $(RTK8189ES_INSTALL_DIR)
	echo $(RTK8189ES_MODULE_DIR)/8189es.ko: >> $(TARGET_DIR)/lib/modules/$(LINUX_VERSION_PROBED)/modules.dep
endef
else
define RTK8189ES_BUILD_CMDS
	mkdir -p $(LINUX_DIR)/../hardware/wifi/realtek/drivers;
	ln -sf $(RTK8189ES_DIR) $(LINUX_DIR)/../hardware/wifi/realtek/drivers/8189es
endef
endif
$(eval $(generic-package))
