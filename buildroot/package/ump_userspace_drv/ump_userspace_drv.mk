#############################################################
#
# UMP User Space Driver
#
#############################################################
#http://malideveloper.arm.com/downloads/drivers/DX910/r4p0-00rel0/DX910-SW-99006-r4p0-00rel0.tgz
UMP_USERSPACE_DRV_VERSION = r4p0-00rel0
UMP_USERSPACE_DRV_SOURCE  = DX910-SW-99006-$(UMP_USERSPACE_DRV_VERSION).tgz
UMP_USERSPACE_DRV_SITE    = http://malideveloper.arm.com/downloads/drivers/DX910/$(UMP_USERSPACE_DRV_VERSION)

define UMP_USERSPACE_DRV_BUILD_CMDS
	$(MAKE) CROSS_COMPILE=$(BR2_TOOLCHAIN_EXTERNAL_PREFIX)-  -C $(@D)/driver/src/ump
endef

define UMP_USERSPACE_DRV_CLEAN_CMDS
	$(MAKE) -C $(@D)/driver/src/ump clean
endef

define UMP_USERSPACE_DRV_INSTALL_TARGET_CMDS
        cp -f $(@D)/driver/src/ump/libUMP.so $(TARGET_DIR)/usr/lib/
        cp -f $(@D)/driver/src/ump/libUMP.so $(STAGING_DIR)/usr/lib/
        cp -f $(@D)/driver/src/ump/libUMP.a $(STAGING_DIR)/usr/lib/
        cp -rf $(@D)/driver/src/ump/include/ump $(STAGING_DIR)/usr/include/
endef

define UMP_USERSPACE_DRV_UNINSTALL_TARGET_CMDS
        rm -f $(TARGET_DIR)/usr/lib/libUMP.so
        rm -f $(STAGING_DIR)/usr/lib/libUMP.a
        rm -f $(STAGING_DIR)/usr/lib/libUMP.so 
        rm -rf $(STAGING_DIR)/usr/include/
endef

$(eval $(generic-package))
