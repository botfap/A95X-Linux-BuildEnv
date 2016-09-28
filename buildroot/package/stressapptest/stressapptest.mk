################################################################################
#
# stress app test
#
################################################################################
STRESSAPPTEST_VERSION = 49
STRESSAPPTEST_SITE = http://stressapptest.googlecode.com/svn/trunk
STRESSAPPTEST_SITE_METHOD = svn

define STRESSAPPTEST_CONFIGURE_CMDS
	(cd $(@D); \
	 ./configure \
	 --host=$(BR2_TOOLCHAIN_EXTERNAL_PREFIX) \
	)
endef

define STRESSAPPTEST_BUILD_CMDS
	$(TARGET_CONFIGURE_OPTS) $(MAKE) -C $(@D)
endef

define STRESSAPPTEST_INSTALL_TARGET_CMDS
	$(INSTALL) -m 0755 $(@D)/src/stressapptest $(TARGET_DIR)/usr/bin
endef

$(eval $(generic-package))
