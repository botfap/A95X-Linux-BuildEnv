#############################################################
#
# gat-app
#
#############################################################
GST_APP_VERSION = 0.11.0
GST_APP_SITE = $(TOPDIR)/package/multimedia/gst-app/gst-app-0.11.0
GST_APP_SITE_METHOD = local

GST_APP_INSTALL_STAGING = YES
GST_APP_AUTORECONF = YES
GST_APP_DEPENDENCIES = gst-aml-plugins

$(eval $(autotools-package))

