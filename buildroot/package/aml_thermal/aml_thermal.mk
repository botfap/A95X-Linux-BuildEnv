################################################################################
#
# aml_thermal
#
################################################################################

AML_THERMAL_VERSION = $(call qstrip,$(BR2_PACKAGE_AML_THERMAL_VERSION))
AML_THERMAL_SITE = $(call qstrip,$(BR2_PACKAGE_AML_THERMAL_URL))
AML_THERMAL_INSTALL_STAGING = NO
AML_THERMAL_DEPENDENCIES = linux
AML_THERMAL_LICENSE = GPLv2+
AML_THERMAL_MODULE_DIR = kernel/amlogic/thermal
AML_THERMAL_INSTALL_DIR = $(TARGET_DIR)/lib/modules/$(LINUX_VERSION_PROBED)/$(AML_THERMAL_MODULE_DIR)
AML_THERMAL_DEPENDENCIES = linux

define AML_THERMAL_BUILD_CMDS
	$(TARGET_CONFIGURE_OPTS) $(MAKE) -C $(LINUX_DIR) M=$(@D) ARCH=$(KERNEL_ARCH) \
		CROSS_COMPILE=$(TARGET_CROSS) modules
endef

define AML_THERMAL_INSTALL_TARGET_CMDS
	mkdir -p $(AML_THERMAL_INSTALL_DIR)
	$(INSTALL) -m 0666 $(@D)/aml_thermal.ko $(AML_THERMAL_INSTALL_DIR)
	echo $(AML_THERMAL_MODULE_DIR)/aml_thermal.ko: >> $(TARGET_DIR)/lib/modules/$(LINUX_VERSION_PROBED)/modules.dep
endef

$(eval $(generic-package))
