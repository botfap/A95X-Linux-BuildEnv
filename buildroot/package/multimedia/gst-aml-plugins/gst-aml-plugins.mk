#############################################################
#
# gat-aml-plugins
#
#############################################################
GST_AML_PLUGINS_VERSION = 0.11.0
#GST_AML_PLUGINS_SITE =file://$(TOPDIR)/package/multimedia/gst-aml-plugins/
GST_AML_PLUGINS_SITE = $(TOPDIR)/package/multimedia/gst-aml-plugins/gst-aml-plugins-0.11.0
GST_AML_PLUGINS_SITE_METHOD = local

GST_AML_PLUGINS_INSTALL_STAGING = YES
GST_AML_PLUGINS_AUTORECONF = YES
GST_AML_PLUGINS_DEPENDENCIES = gstreamer host-pkgconf libplayer

$(eval $(autotools-package))

