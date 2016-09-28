################################################################################
#
# amlogic af_alg
#
################################################################################

AML_AF_ALG_VERSION = $(call qstrip,$(BR2_PACKAGE_AML_AF_ALG_VERSION))
AML_AF_ALG_SITE = $(call qstrip,$(BR2_PACKAGE_AML_AF_ALG_GIT_URL))
AML_AF_ALG_LICENSE = OpenSSL
AML_AF_ALG_DEPENDENCIES = openssl
AML_AF_ALG_INSTALL_STAGING = YES
AML_AF_ALG_CFLAGS = $(TARGET_CFLAGS)

define AML_AF_ALG_BUILD_CMDS
	$(MAKE1) CC=$(TARGET_CC) -C $(@D)
endef

define AML_AF_ALG_INSTALL_TARGET_CMDS
	mkdir -p $(TARGET_DIR)/usr/lib/engines
	install -m 0755 $(@D)/libaf_alg.so $(TARGET_DIR)/usr/lib/engines
endef

$(eval $(generic-package))
