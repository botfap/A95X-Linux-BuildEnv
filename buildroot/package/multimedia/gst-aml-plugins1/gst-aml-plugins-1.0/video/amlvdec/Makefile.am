# Note: plugindir is set in configure

##############################################################################
# TODO: change libgstplugin.la to something else, e.g. libmysomething.la     #
##############################################################################
plugin_LTLIBRARIES = libgstamlvdec.la

##############################################################################
# TODO: for the next set of variables, name the prefix if you named the .la, #
#  e.g. libmysomething.la => libmysomething_la_SOURCES                       #
#                            libmysomething_la_CFLAGS                        #
#                            libmysomething_la_LIBADD                        #
#                            libmysomething_la_LDFLAGS                       #
##############################################################################

# sources used to compile this plug-in
libgstamlvdec_la_SOURCES = gstamlvdec.c gstamlvdec.h  amlvideoinfo.c

# compiler and linker flags used to compile this plugin, set in configure.ac
libgstamlvdec_la_CFLAGS = $(GST_CFLAGS) -I ../../common/amlsysctl -I../../common/amstreaminfo -I../../common/include
libgstamlvdec_la_LIBADD = $(GST_LIBS) -lgstvideo-1.0 
libgstamlvdec_la_LIBADD += ../../common/libcommon.a  -L$(TARGET_DIR)/usr/lib -lamcodec -lamadec -lamavutils -lamplayer
libgstamlvdec_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)
libgstamlvdec_la_LIBTOOLFLAGS = --tag=disable-static

# headers we need but don't want installed
noinst_HEADERS = gstamlvdec.h 