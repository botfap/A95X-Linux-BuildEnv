################################################################################
#
# xdriver_xf86-video-mali -- video driver for mali 
#
################################################################################

XDRIVER_XF86_VIDEO_MALI_VERSION = amlogic
XDRIVER_XF86_VIDEO_MALI_SITE = $(TOPDIR)/package/x11r7/xdriver_xf86-video-mali/src
XDRIVER_XF86_VIDEO_MALI_SITE_METHOD = local
XDRIVER_XF86_VIDEO_MALI_DEPENDENCIES = xserver_xorg-server xproto_xf86driproto xproto_fontsproto xproto_randrproto xproto_renderproto xproto_videoproto xproto_xproto
XDRIVER_XF86_VIDEO_MALI_AUTORECONF = YES
XDRIVER_XF86_VIDEO_MALI_CONF_OPTS += CPPFLAGS="-I$(@D)/ddk-include/include"

define XDRIVER_XF86_VIDEO_MALI_CHANGE_MODE
	chmod +x $(@D)/configure
endef
XDRIVER_XF86_VIDEO_MALI_POST_RSYNC_HOOKS += XDRIVER_XF86_VIDEO_MALI_CHANGE_MODE

$(eval $(autotools-package))
