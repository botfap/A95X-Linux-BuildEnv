################################################################################
#
# amlogic customer
#
################################################################################

AML_CUSTOMER_VERSION = $(call qstrip,$(BR2_PACKAGE_AML_CUSTOMER_VERSION))
AML_CUSTOMER_SITE = $(call qstrip,$(BR2_PACKAGE_AML_CUSTOMER_GIT_URL))
AML_CUSTOMER_LICENSE = GPLv2+
AML_CUSTOMER_LICENSE_FILES = COPYING.GPL

define AML_CUSTOMER_BUILD_CMDS
endef

define AML_CUSTOMER_INSTALL_TARGET_CMDS
endef

$(eval $(generic-package))
