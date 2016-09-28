################################################################################
#
# mtk7601 driver
#
################################################################################

MTK7601_VERSION = $(call qstrip,$(BR2_PACKAGE_MTK7601_GIT_VERSION))
MTK7601_SITE = $(call qstrip,$(BR2_PACKAGE_MTK7601_GIT_REPO_URL))
MTK7601_MODULE_DIR = kernel/mtk/wifi
MTK7601_INSTALL_DIR = $(TARGET_DIR)/lib/modules/$(LINUX_VERSION_PROBED)/$(MTK7601_MODULE_DIR)
MTK7601_DEPENDENCIES = linux

define MTK7601_BUILD_CMDS
	$(TARGET_CONFIGURE_OPTS) $(MAKE) -C $(LINUX_DIR) M=$(@D) ARCH=$(KERNEL_ARCH) \
		CROSS_COMPILE=$(TARGET_CROSS) modules
endef
define MTK7601_INSTALL_TARGET_CMDS
	mkdir -p $(MTK7601_INSTALL_DIR)
	$(INSTALL) -m 0666 $(@D)/mt7601usta.ko $(MTK7601_INSTALL_DIR)
	echo $(MTK7601_MODULE_DIR)/mt7601usta.ko: >> $(TARGET_DIR)/lib/modules/$(LINUX_VERSION_PROBED)/modules.dep
	$(INSTALL) -m 0666 $(@D)/mtprealloc.ko $(MTK7601_INSTALL_DIR)
	echo $(MTK7601_MODULE_DIR)/mtprealloc.ko: >> $(TARGET_DIR)/lib/modules/$(LINUX_VERSION_PROBED)/modules.dep
endef
$(eval $(generic-package))
