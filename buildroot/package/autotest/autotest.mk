################################################################################
#
# autotest
#
################################################################################
AUTOTEST_VERSION = master
AUTOTEST_SITE = $(call github,autotest,autotest,$(AUTOTEST_VERSION))
AUTOTEST_LICENSE = LGPLv3+
AUTOTEST_LICENSE_FILES = LGPL_LICENSE
AUTOTEST_DEPENDENCIES = host-python host-python-sphinx
AUTOTEST_SETUP_TYPE = distutils

define AUTOTEST_ENV
	export AUTOTEST_TOP_PATH=$(TARGET_DIR)/etc/autotest; 
endef

$(eval $(python-package))
