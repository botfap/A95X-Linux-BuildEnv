#############################################################
#
# Amlogic Trustzone trusted 
#
#############################################################
AML_BDK_VERSION = 2.2
AML_BDK_SITE = $(TOPDIR)/package/aml_bdk/src
AML_BDK_SITE_METHOD = local
AML_BDK_DEPENDENCIES = linux

define AML_BDK_BUILD_CMDS
	$(TARGET_CONFIGURE_OPTS) $(MAKE1) -C $(@D) all
	export KERNEL_SRC_DIR=$(LINUX_DIR); \
	export KERNEL_BUILT_DIR=$(LINUX_DIR); \
	export KERNEL_CONFIG=$(BR2_LINUX_KERNEL_DEFCONFIG)_defconfig; \
	$(TARGET_CONFIGURE_OPTS) $(MAKE1) -C $(@D) driver
endef

define AML_BDK_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/ca/lib/*.so $(TARGET_DIR)/usr/lib/
	rm $(@D)/ca/out/apps/*.o 
	$(INSTALL) -D -m 0755 $(@D)/ca/out/apps/* $(TARGET_DIR)/usr/bin/
	$(INSTALL) -D -m 0644 $(@D)/ta/out/*.tzo $(TARGET_DIR)/usr/lib/
	mkdir -p $(TARGET_DIR)/lib/modules/$(LINUX_VERSION_PROBED)/kernel/drivers/trustzone/
	$(INSTALL) -D -m 0644 $(@D)/driver/*.ko $(TARGET_DIR)/lib/modules/$(LINUX_VERSION_PROBED)/kernel/drivers/trustzone/
endef

define AML_BDK_CLEAN_CMDS
	$(TARGET_CONFIGURE_OPTS) $(MAKE) -C $(@D) clean
endef

$(eval $(generic-package))
