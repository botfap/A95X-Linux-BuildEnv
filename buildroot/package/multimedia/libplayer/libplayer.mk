#############################################################
#
# libplayer
#
#############################################################
LIBPLAYER_VERSION:=2.1.0
LIBPLAYER_SITE=$(TOPDIR)/package/multimedia/libplayer/src
LIBPLAYER_SITE_METHOD=local
LIBPLAYER_BUILD_DIR = $(BUILD_DIR)
LIBPLAYER_INSTALL_STAGING = YES
LIBPLAYER_DEPENDENCIES = zlib alsa-lib

export LIBPLAYER_STAGING_DIR = $(STAGING_DIR)
export LIBPLAYER_TARGET_DIR = $(TARGET_DIR)

AMFFMPEG_CONF_OPTS = \
		--disable-yasm \
		--enable-debug \
		--disable-ffplay \
		--disable-ffmpeg \
		--enable-cross-compile \
		--target-os=linux \
		--disable-librtmp \
		--disable-static \
		--enable-shared \
		--disable-ffserver \
		--disable-doc

ifneq ($(call qstrip,$(BR2_GCC_TARGET_CPU)),)
AMFFMPEG_CONF_OPTS += --cpu=$(BR2_GCC_TARGET_CPU)
else ifneq ($(call qstrip,$(BR2_GCC_TARGET_ARCH)),)
AMFFMPEG_CONF_OPTS += --cpu=$(BR2_GCC_TARGET_ARCH)
endif

define LIBPLAYER_CONFIGURE_CMDS
		cd $(@D)/amffmpeg; \
		$(TARGET_MAKE_ENV) \
		./configure \
		--prefix=$(STAGING_DIR)/usr \
		--shlibdir=$(STAGING_DIR)/usr/lib/libplayer \
		--cross-prefix=$(BR2_TOOLCHAIN_EXTERNAL_PREFIX)- \
		--arch=$(BR2_ARCH) \
		--extra-ldflags='-L$(STAGING_DIR)/usr/lib/ -L$(STAGING_DIR)/usr/lib/libplayer -lamavutils -ldl' \
		$(AMFFMPEG_CONF_OPTS) \

endef

define LIBPLAYER_BUILD_CMDS
	$(TARGET_MAKE_ENV) $(MAKE) CC=$(TARGET_CC) -C $(@D) all
	$(TARGET_MAKE_ENV) $(MAKE) CC=$(TARGET_CC) -C $(@D) install
endef

define LIBPLAYER_CLEAN_CMDS
	$(MAKE) -C $(@D) clean
endef

$(eval $(generic-package))
