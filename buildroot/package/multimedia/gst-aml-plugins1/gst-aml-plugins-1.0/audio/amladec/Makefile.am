# Note: plugindir is set in configure

##############################################################################
# TODO: change libgstplugin.la to something else, e.g. libmysomething.la     #
##############################################################################
plugin_LTLIBRARIES = libgstamladec.la

##############################################################################
# TODO: for the next set of variables, name the prefix if you named the .la, #
#  e.g. libmysomething.la => libmysomething_la_SOURCES                       #
#                            libmysomething_la_CFLAGS                        #
#                            libmysomething_la_LIBADD                        #
#                            libmysomething_la_LDFLAGS                       #
##############################################################################

# sources used to compile this plug-in
libgstamladec_la_SOURCES = gstamladec.c gstamladec.h amlaudioinfo.c

# compiler and linker flags used to compile this plugin, set in configure.ac
libgstamladec_la_CFLAGS = $(GST_CFLAGS) -I../../common/amlsysctl -I../../common/amstreaminfo
libgstamladec_la_LIBADD = $(GST_LIBS) -lgstaudio-1.0
libgstamladec_la_LIBADD += ../../common/libcommon.a -L$(TARGET_DIR)/usr/lib -lamcodec -lamadec -lamavutils -lamplayer
libgstamladec_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)
libgstamladec_la_LIBTOOLFLAGS = --tag=disable-static

# headers we need but don't want installed
noinst_HEADERS = gstamladec.h 
