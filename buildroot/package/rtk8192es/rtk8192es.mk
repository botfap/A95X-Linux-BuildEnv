################################################################################
#
# amlogic 8192es driver
#
################################################################################

RTK8192ES_VERSION = $(call qstrip,$(BR2_PACKAGE_RTK8192ES_GIT_VERSION))
RTK8192ES_SITE = $(call qstrip,$(BR2_PACKAGE_RTK8192ES_GIT_REPO_URL))
RTK8192ES_MODULE_DIR = kernel/realtek/wifi
RTK8192ES_INSTALL_DIR = $(TARGET_DIR)/lib/modules/$(LINUX_VERSION_PROBED)/$(RTK8192ES_MODULE_DIR)
ifeq ($(BR2_PACKAGE_RTK8192ES_STANDALONE),y)
RTK8192ES_DEPENDENCIES = linux
endif

ifeq ($(BR2_PACKAGE_RTK8192ES_STANDALONE),y)
define RTK8192ES_BUILD_CMDS
	$(TARGET_CONFIGURE_OPTS) $(MAKE) -C $(LINUX_DIR) M=$(@D)/rtl8192ES ARCH=$(KERNEL_ARCH) \
		CROSS_COMPILE=$(TARGET_CROSS) modules
endef
define RTK8192ES_INSTALL_TARGET_CMDS
	mkdir -p $(RTK8192ES_INSTALL_DIR)
	$(INSTALL) -m 0666 $(@D)/rtl8192ES/8192es.ko $(RTK8192ES_INSTALL_DIR)
	echo $(RTK8192ES_MODULE_DIR)/8192es.ko: >> $(TARGET_DIR)/lib/modules/$(LINUX_VERSION_PROBED)/modules.dep
endef
else
define RTK8192ES_BUILD_CMDS
	mkdir -p $(LINUX_DIR)/../hardware/wifi/realtek/drivers;
	ln -sf $(RTK8192ES_DIR) $(LINUX_DIR)/../hardware/wifi/realtek/drivers/8192es
endef
endif
$(eval $(generic-package))
