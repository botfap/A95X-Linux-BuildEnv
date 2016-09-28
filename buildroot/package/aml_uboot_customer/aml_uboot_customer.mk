#############################################################
#
# Amlogic UBOOT Customer
#
#############################################################
AML_UBOOT_CUSTOMER_VERSION = $(call qstrip,$(BR2_PACKAGE_AML_UBOOT_CUSTOMER_GIT_VERSION))
AML_UBOOT_CUSTOMER_SITE = $(call qstrip,$(BR2_PACKAGE_AML_UBOOT_CUSTOMER_GIT_REPO_URL))

$(eval $(generic-package))
