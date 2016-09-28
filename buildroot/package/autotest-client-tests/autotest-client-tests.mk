################################################################################
#
# autotest-client-tests
#
################################################################################
AUTOTEST_CLIENT_TESTS_VERSION = master
AUTOTEST_CLIENT_TESTS_SITE = $(call github,autotest,autotest-client-tests,$(AUTOTEST_VERSION))
AUTOTEST_CLIENT_TESTS_LICENSE = LGPLv3+
AUTOTEST_CLIENT_TESTS_LICENSE_FILES = LGPL_LICENSE
AUTOTEST_CLIENT_TESTS_DEPENDENCIES = autotest

define AUTOTEST_CLIENT_TESTS_INSTALL_TARGET_CMDS
	mkdir -p $(TARGET_DIR)/usr/lib/python2.7/site-packages/autotest/client/tests
	cp -rf $(@D)/* $(TARGET_DIR)/usr/lib/python2.7/site-packages/autotest/client/tests
endef

$(eval $(generic-package))
