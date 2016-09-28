################################################################################
#
# amlogic broadcom AP6xxx driver
#
################################################################################

BRCMAP6XXX_VERSION = $(call qstrip,$(BR2_PACKAGE_BRCMAP6XXX_GIT_VERSION))
BRCMAP6XXX_SITE = $(call qstrip,$(BR2_PACKAGE_BRCMAP6XXX_GIT_REPO_URL))
BRCMAP6XXX_MODULE_DIR = kernel/broadcom/wifi
BRCMAP6XXX_INSTALL_DIR = $(TARGET_DIR)/lib/modules/$(LINUX_VERSION_PROBED)/$(BRCMAP6XXX_MODULE_DIR)
ifeq ($(BR2_PACKAGE_BRCMAP6XXX_STANDALONE),y)
BRCMAP6XXX_DEPENDENCIES = linux
endif
BRCMAP6XXX_KCONFIGS = KCPPFLAGS='-DCONFIG_BCMDHD_FW_PATH=\"/etc/wifi/fw_bcmdhd.bin\" -DCONFIG_BCMDHD_NVRAM_PATH=\"/etc/wifi/nvram.txt\"'

ifeq ($(BR2_PACKAGE_BRCMAP6XXX_STANDALONE),y)
define BRCMAP6XXX_BUILD_CMDS
	$(TARGET_CONFIGURE_OPTS) $(MAKE) -C $(LINUX_DIR) M=$(@D)/bcmdhd_1_201_59_x ARCH=$(KERNEL_ARCH) \
		CROSS_COMPILE=$(TARGET_CROSS) $(BRCMAP6XXX_KCONFIGS) modules
	$(TARGET_CONFIGURE_OPTS) $(MAKE) -C $(LINUX_DIR) M=$(@D)/bcmdhd-usb.1.201.88.27.x ARCH=$(KERNEL_ARCH) \
		CROSS_COMPILE=$(TARGET_CROSS) $(BRCMAP6XXX_KCONFIGS) modules
endef
define BRCMAP6XXX_INSTALL_TARGET_CMDS
	mkdir -p $(BRCMAP6XXX_INSTALL_DIR)
	$(INSTALL) -m 0666 $(@D)/bcmdhd_1_201_59_x/dhd.ko $(BRCMAP6XXX_INSTALL_DIR)
	echo $(BRCMAP6XXX_MODULE_DIR)/dhd.ko: >> $(TARGET_DIR)/lib/modules/$(LINUX_VERSION_PROBED)/modules.dep
	$(INSTALL) -m 0666 $(@D)/bcmdhd-usb.1.201.88.27.x/bcmdhd.ko $(BRCMAP6XXX_INSTALL_DIR)
	echo $(BRCMAP6XXX_MODULE_DIR)/bcmdhd.ko: >> $(TARGET_DIR)/lib/modules/$(LINUX_VERSION_PROBED)/modules.dep
endef
else
define BRCMAP6XXX_BUILD_CMDS
	mkdir -p $(LINUX_DIR)/../hardware/wifi/broadcom/drivers;
	ln -sf $(BRCMAP6XXX_DIR) $(LINUX_DIR)/../hardware/wifi/broadcom/drivers/ap6xxx
endef
endif


$(eval $(generic-package))
