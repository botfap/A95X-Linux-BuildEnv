################################################################################
#
# mtk7603 driver
#
################################################################################

MTK7603_VERSION = $(call qstrip,$(BR2_PACKAGE_MTK7603_GIT_VERSION))
MTK7603_SITE = $(call qstrip,$(BR2_PACKAGE_MTK7603_GIT_REPO_URL))
MTK7603_MODULE_DIR = kernel/mtk/wifi
MTK7603_INSTALL_DIR = $(TARGET_DIR)/lib/modules/$(LINUX_VERSION_PROBED)/$(MTK7603_MODULE_DIR)
MTK7603_DEPENDENCIES = linux

define MTK7603_BUILD_CMDS
	$(TARGET_CONFIGURE_OPTS) $(MAKE) CROSS_COMPILE=$(TARGET_CROSS) LINUX_SRC=$(LINUX_DIR) \
		RT28xx_DIR=$(@D) -f $(@D)/Makefile
endef
define MTK7603_INSTALL_TARGET_CMDS
	mkdir -p $(MTK7603_INSTALL_DIR)
	$(INSTALL) -m 0666 $(@D)/os/linux/mt7603usta.ko $(MTK7603_INSTALL_DIR)
	echo $(MTK7603_MODULE_DIR)/mt7603usta.ko: >> $(TARGET_DIR)/lib/modules/$(LINUX_VERSION_PROBED)/modules.dep
	$(INSTALL) -m 0666 $(@D)/os/linux/mtprealloc.ko $(MTK7603_INSTALL_DIR)
	echo $(MTK7603_MODULE_DIR)/mtprealloc.ko: >> $(TARGET_DIR)/lib/modules/$(LINUX_VERSION_PROBED)/modules.dep
endef
$(eval $(generic-package))
