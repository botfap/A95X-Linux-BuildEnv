################################################################################
#
# amlogic image packer
#
################################################################################
AML_IMAGE_PACKER_VERSION = 2013
AML_IMAGE_PACKER_SITE = $(TOPDIR)/package/aml_image_packer/src
AML_IMAGE_PACKER_SITE_METHOD = local

WORD_NUMBER := $(words $(BR2_LINUX_KERNEL_INTREE_DTS_NAME))
DTBNAME := dtb.img

define HOST_AML_IMAGE_PACKER_BUILD_CMDS
	$(MAKE) -C $(@D) image_packer
endef

define HOST_AML_IMAGE_PACKER_INSTALL_CMDS
	$(INSTALL) -m 0755 $(@D)/aml_image_v2_packer $(HOST_DIR)/usr/bin
	echo '#This file define how pack aml_upgrade_package image' > $(BINARIES_DIR)/usb_burn_package.conf
	echo "" >> $(BINARIES_DIR)/usb_burn_package.conf
	echo '[LIST_NORMAL]' >>  $(BINARIES_DIR)/usb_burn_package.conf
	echo "#partition images, don't need verfiy" >> $(BINARIES_DIR)/usb_burn_package.conf
	echo 'file="ddr_init.bin"     main_type= "USB"           sub_type="DDR"' >> $(BINARIES_DIR)/usb_burn_package.conf
	echo 'file="u-boot-comp.bin"  main_type= "USB"           sub_type="UBOOT_COMP"' >> $(BINARIES_DIR)/usb_burn_package.conf
	echo 'file="platform.conf"    main_type= "conf"          sub_type="platform"' >> $(BINARIES_DIR)/usb_burn_package.conf
	echo "file=\"$(DTBNAME)\"        main_type=\"dtb\"           sub_type=\"meson\"" >> $(BINARIES_DIR)/usb_burn_package.conf
	echo "" >> $(BINARIES_DIR)/usb_burn_package.conf
	echo '[LIST_VERIFY]' >> $(BINARIES_DIR)/usb_burn_package.conf
	echo '#partition images needed verify'  >> $(BINARIES_DIR)/usb_burn_package.conf
	echo 'file="boot.img"         main_type="PARTITION"      sub_type="boot"' >> $(BINARIES_DIR)/usb_burn_package.conf
	echo 'file="rootfs.ext2"       main_type="PARTITION"      sub_type="system"' >> $(BINARIES_DIR)/usb_burn_package.conf
	echo 'file="u-boot.bin"       main_type="PARTITION"      sub_type="bootloader"' >> $(BINARIES_DIR)/usb_burn_package.conf

	echo "" > $(BINARIES_DIR)/keys.conf
endef

$(eval $(host-generic-package))
