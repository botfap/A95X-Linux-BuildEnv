# Note: plugindir is set in configure

##############################################################################
# TODO: change libgstplugin.la to something else, e.g. libmysomething.la     #
##############################################################################
plugin_LTLIBRARIES = libgstamlvsink.la

##############################################################################
# TODO: for the next set of variables, name the prefix if you named the .la, #
#  e.g. libmysomething.la => libmysomething_la_SOURCES                       #
#                            libmysomething_la_CFLAGS                        #
#                            libmysomething_la_LIBADD                        #
#                            libmysomething_la_LDFLAGS                       #
##############################################################################

# sources used to compile this plug-in
libgstamlvsink_la_SOURCES = gstamlvsink.c gstamlvsink.h amlvsink_prop.c amlvsink_prop.h rectangleInfo.c rectangleInfo.h

# compiler and linker flags used to compile this plugin, set in configure.ac
libgstamlvsink_la_CFLAGS = $(GST_CFLAGS) -I../../common/amlsysctl -I../../common/amstreaminfo
libgstamlvsink_la_LIBADD = $(GST_LIBS) 
libgstamlvsink_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)
libgstamlvsink_la_LIBADD += ../../common/libcommon.a
libgstamlvsink_la_LIBTOOLFLAGS = --tag=disable-static

# headers we need but don't want installed
noinst_HEADERS = gstamlvsink.h amlvsink_prop.h rectangleInfo.h