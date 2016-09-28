################################################################################
#
# amlogic gpu driver
#
################################################################################
GPU_VERSION = $(call qstrip,$(BR2_PACKAGE_GPU_VERSION))
ifneq ($(BR2_PACKAGE_GPU_CUSTOM_TARBALL_LOCATION),"")
GPU_TARBALL = $(call qstrip,$(BR2_PACKAGE_GPU_CUSTOM_TARBALL_LOCATION))
GPU_SITE    = $(patsubst %/,%,$(dir $(GPU_TARBALL)))
GPU_SOURCE  = $(notdir $(GPU_TARBALL))
else
GPU_SITE = $(call qstrip,$(BR2_PACKAGE_GPU_GIT_URL))
GPU_SITE_METHOD = git
endif
GPU_MODULE_DIR = kernel/amlogic/gpu
GPU_INSTALL_DIR = $(TARGET_DIR)/lib/modules/$(LINUX_VERSION_PROBED)/$(GPU_MODULE_DIR)
ifeq ($(BR2_PACKAGE_GPU_STANDALONE),y)
GPU_DEPENDENCIES = linux
endif

ifeq ($(BR2_PACKAGE_GPU_MALI_DRM),y)
MALI_DRM_BUILD_CMD = \
	$(TARGET_CONFIGURE_OPTS) $(MAKE) -C $(LINUX_DIR) M=$(LINUX_DIR)/drivers/gpu/drm/mali_drm ARCH=$(KERNEL_ARCH) \
		CROSS_COMPILE=$(TARGET_CROSS) modules
MALI_DRM_INSTALL_TARGET_CMDS = \
	$(INSTALL) -m 0666 $(LINUX_DIR)/drivers/gpu/drm/mali_drm/mali_drm.ko $(GPU_INSTALL_DIR); \
	echo $(GPU_MODULE_DIR)/mali_drm.ko: >> $(TARGET_DIR)/lib/modules/$(LINUX_VERSION_PROBED)/modules.dep
endif
ifeq ($(BR2_PACKAGE_GPU_UMP),y)
MALI_BUILD_CMD = \
	$(TARGET_CONFIGURE_OPTS) $(MAKE) -C $(LINUX_DIR) M=$(@D)/mali ARCH=$(KERNEL_ARCH) \
		CROSS_COMPILE=$(TARGET_CROSS) CONFIG_MALI400=m CONFIG_MALI450=m MALI400_UMP=y modules;
MALI_UMP_BUILD_CMD = \
	$(TARGET_CONFIGURE_OPTS) $(MAKE) -C $(LINUX_DIR) M=$(@D)/ump ARCH=$(KERNEL_ARCH) \
		CROSS_COMPILE=$(TARGET_CROSS) CONFIG_UMP=m modules
MALI_UMP_INSTALL_TARGET_CMDS = \
	$(INSTALL) -m 0666 $(@D)/ump/ump.ko $(GPU_INSTALL_DIR); \
	echo $(GPU_MODULE_DIR)/ump.ko: >> $(TARGET_DIR)/lib/modules/$(LINUX_VERSION_PROBED)/modules.dep;
else
ifeq ($(BR2_PACKAGE_GPU_MALI_DMA_SHARED_BUFFER),y)
MALI_BUILD_CMD = \
	$(TARGET_CONFIGURE_OPTS) $(MAKE) -C $(LINUX_DIR) M=$(@D)/mali ARCH=$(KERNEL_ARCH) \
		CROSS_COMPILE=$(TARGET_CROSS) CONFIG_MALI400=m CONFIG_MALI450=m CONFIG_DMA_SHARED_BUFFER=y modules
else
MALI_BUILD_CMD = \
	$(TARGET_CONFIGURE_OPTS) $(MAKE) -C $(LINUX_DIR) M=$(@D)/mali ARCH=$(KERNEL_ARCH) \
		CROSS_COMPILE=$(TARGET_CROSS) CONFIG_MALI400=m CONFIG_MALI450=m  \
		EXTRA_CFLAGS="-DCONFIG_MALI400=m -DCONFIG_MALI450=m" modules
endif
endif
MALI_INSTALL_TARGETS_CMDS = \
	$(INSTALL) -m 0666 $(@D)/mali/mali.ko $(GPU_INSTALL_DIR); \
	echo $(GPU_MODULE_DIR)/mali.ko: >> $(TARGET_DIR)/lib/modules/$(LINUX_VERSION_PROBED)/modules.dep;

ifeq ($(BR2_PACKAGE_GPU_STANDALONE),y)
define GPU_BUILD_CMDS
	$(MALI_BUILD_CMD)
	$(MALI_UMP_BUILD_CMD) 
	$(MALI_DRM_BUILD_CMD)
endef
define GPU_INSTALL_TARGET_CMDS
	mkdir -p $(GPU_INSTALL_DIR);
	$(MALI_INSTALL_TARGETS_CMDS)
	$(MALI_UMP_INSTALL_TARGET_CMDS) 
	$(MALI_DRM_INSTALL_TARGET_CMDS)
endef
else
define GPU_BUILD_CMDS
	mkdir -p  $(LINUX_DIR)/../hardware/arm;
	ln -sf $(GPU_DIR) $(LINUX_DIR)/../hardware/arm/gpu
endef
endif
$(eval $(generic-package))

