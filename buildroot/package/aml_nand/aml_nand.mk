################################################################################
#
# amlogic nand ftl
#
################################################################################

AML_NAND_VERSION = $(call qstrip,$(BR2_PACKAGE_AML_NAND_VERSION))
AML_NAND_SITE = $(call qstrip,$(BR2_PACKAGE_AML_NAND_GIT_URL))
AML_NAND_MODULE_DIR = kernel/amlogic/nand
AML_NAND_INSTALL_DIR = $(TARGET_DIR)/lib/modules/$(LINUX_VERSION_PROBED)/$(AML_NAND_MODULE_DIR)
ifeq ($(BR2_PACKAGE_AML_NAND_STANDALONE),y)
AML_NAND_DEPENDENCIES = linux
endif

ifeq ($(BR2_PACKAGE_AML_NAND_STANDALONE),y)
define AML_NAND_BUILD_CMDS
	$(TARGET_CONFIGURE_OPTS) $(MAKE) -C $(LINUX_DIR) M=$(@D)/amlnf_3.14 ARCH=$(KERNEL_ARCH) \
		EXTRA_CFLAGS+='-I$(@D)/amlnf/include -I$(@D)/amlnf/ntd' CROSS_COMPILE=$(TARGET_CROSS) modules
endef
define AML_NAND_INSTALL_TARGET_CMDS
	mkdir -p $(AML_NAND_INSTALL_DIR);
	$(INSTALL) -m 0666 $(@D)/amlnf_3.14/aml_nftl_dev.ko $(AML_NAND_INSTALL_DIR); \
	echo $(AML_NAND_MODULE_DIR)/aml_nftl_dev.ko: >> $(TARGET_DIR)/lib/modules/$(LINUX_VERSION_PROBED)/modules.dep;
endef
else
define AML_NAND_BUILD_CMDS
	mkdir -p  $(LINUX_DIR)/../hardware/amlogic;
	ln -sf $(AML_NAND_DIR) $(LINUX_DIR)/../hardware/amlogic/nand
endef
endif

$(eval $(generic-package))
