################################################################################
#
# Build the ext2 root filesystem image
#
################################################################################
EXT2_OPTS = -G $(BR2_TARGET_ROOTFS_EXT2_GEN) -R $(BR2_TARGET_ROOTFS_EXT2_REV)

ifneq ($(strip $(BR2_TARGET_ROOTFS_EXT2_BLOCKS)),0)
EXT2_OPTS += -b $(BR2_TARGET_ROOTFS_EXT2_BLOCKS)
endif
EXT2_OPTS += -B $(BR2_TARGET_ROOTFS_EXT2_EXTRA_BLOCKS)

ifneq ($(strip $(BR2_TARGET_ROOTFS_EXT2_INODES)),0)
EXT2_OPTS += -i $(BR2_TARGET_ROOTFS_EXT2_INODES)
endif
EXT2_OPTS += -I $(BR2_TARGET_ROOTFS_EXT2_EXTRA_INODES)

ifneq ($(strip $(BR2_TARGET_ROOTFS_EXT2_RESBLKS)),0)
EXT2_OPTS += -r $(BR2_TARGET_ROOTFS_EXT2_RESBLKS)
endif

# qstrip it when checking. Furthermore, we need to further quote it, so
# that the quotes do not get eaten by the echo statement when creating the
# fakeroot script
ifneq ($(call qstrip,$(BR2_TARGET_ROOTFS_EXT2_LABEL)),)
EXT2_OPTS += -l '$(BR2_TARGET_ROOTFS_EXT2_LABEL)'
endif

ROOTFS_EXT2_DEPENDENCIES = host-mke2img host-parted

define ROOTFS_EXT2_CMD
	PATH=$(BR_PATH) mke2img -d $(TARGET_DIR) $(EXT2_OPTS) -o $@
endef

rootfs-ext2-symlink:
	ln -sf rootfs.ext2$(ROOTFS_EXT2_COMPRESS_EXT) $(BINARIES_DIR)/rootfs.ext$(BR2_TARGET_ROOTFS_EXT2_GEN)$(ROOTFS_EXT2_COMPRESS_EXT)

.PHONY: rootfs-ext2-symlink

ifneq ($(BR2_TARGET_ROOTFS_EXT2_GEN),2)
ROOTFS_EXT2_POST_TARGETS += rootfs-ext2-symlink
endif

DEVICE_DIR := $(patsubst "%",%,$(BR2_ROOTFS_OVERLAY))
UPGRADE_DIR := $(DEVICE_DIR)../upgrade
ifeq ($(BR2_TARGET_USBTOOL_AMLOGIC),y)
ifeq ($(BR2_TARGET_UBOOT_AMLOGIC_2015),y)
rootfs-usb-image-pack:
	cp -rf $(UPGRADE_DIR)/* $(BINARIES_DIR)
	BINARIES_DIR=$(BINARIES_DIR) \
	TOOL_DIR=$(HOST_DIR)/usr/bin \
	$(HOST_DIR)/usr/bin/aml_upgrade_pkg_gen.sh \
	$(BR2_TARGET_UBOOT_PLATFORM) $(BR2_TARGET_UBOOT_ENCRYPTION)
else #BR2_TARGET_UBOOT_AMLOGIC_2015
rootfs-usb-image-pack:
	cp -f $(UPGRADE_DIR)/platform.conf $(BINARIES_DIR)
	$(HOST_DIR)/usr/bin/aml_image_v2_packer -r $(BINARIES_DIR)/usb_burn_package.conf $(BINARIES_DIR)/ $(BINARIES_DIR)/aml_upgrade_package.img
endif #BR2_TARGET_UBOOT_AMLOGIC_2015
ROOTFS_EXT2_POST_TARGETS += rootfs-usb-image-pack
endif #BR2_TARGET_USBTOOL_AMLOGIC

ifeq ($(BR2_TARGET_UBOOT_AMLOGIC_2015),y)
SD_BOOT = $(BINARIES_DIR)/u-boot.bin.sd.bin
else
SD_BOOT = $(BINARIES_DIR)/u-boot.bin
endif
mbr-image:
	@$(call MESSAGE,"Creating mbr image")
	filesz=`stat -c '%s' $(BINARIES_DIR)/rootfs.ext2`; \
	dd if=/dev/zero of=$(BINARIES_DIR)/disk.img bs=512 count=$$(($$filesz/512+2048)); \
	parted $(BINARIES_DIR)/disk.img mklabel msdos; \
	parted $(BINARIES_DIR)/disk.img mkpart primary ext2 2048s 100%; \
	dd if=$(BINARIES_DIR)/rootfs.ext2 of=$(BINARIES_DIR)/disk.img bs=512 count=$$(($$filesz/512)) seek=2048;

odroid-uboot:
	dd if=$(BINARIES_DIR)/bl1.bin.hardkernel of=$(BINARIES_DIR)/disk.img bs=1 count=442 conv=notrunc; \
	dd if=$(BINARIES_DIR)/bl1.bin.hardkernel of=$(BINARIES_DIR)/disk.img bs=512 skip=1 seek=1 conv=notrunc; \
	dd if=$(BINARIES_DIR)/u-boot.bin of=$(BINARIES_DIR)/disk.img bs=512 seek=64 conv=notrunc;

amlogic-uboot:
	dd if=$(SD_BOOT) of=$(BINARIES_DIR)/disk.img bs=1 count=442 conv=notrunc; \
	dd if=$(SD_BOOT) of=$(BINARIES_DIR)/disk.img bs=512 skip=1 seek=1 conv=notrunc;

ifeq ($(BR2_TARGET_MBR_IMAGE),y)
ROOTFS_EXT2_POST_TARGETS += mbr-image
ifeq ($(BR2_TARGET_UBOOT_ODROID),y)
ROOTFS_EXT2_POST_TARGETS += odroid-uboot
else
ROOTFS_EXT2_POST_TARGETS += amlogic-uboot
endif
endif

$(eval $(call ROOTFS_TARGET,ext2))
