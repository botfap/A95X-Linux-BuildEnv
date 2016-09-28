# Note: plugindir is set in configure

##############################################################################
# TODO: change libgstplugin.la to something else, e.g. libmysomething.la     #
##############################################################################
plugin_LTLIBRARIES = libgstamlasink.la

##############################################################################
# TODO: for the next set of variables, name the prefix if you named the .la, #
#  e.g. libmysomething.la => libmysomething_la_SOURCES                       #
#                            libmysomething_la_CFLAGS                        #
#                            libmysomething_la_LIBADD                        #
#                            libmysomething_la_LDFLAGS                       #
##############################################################################

# sources used to compile this plug-in
libgstamlasink_la_SOURCES = gstamlasink.c gstamlasink.h amlasink_prop.c amlasink_prop.h

# compiler and linker flags used to compile this plugin, set in configure.ac
libgstamlasink_la_CFLAGS = $(GST_CFLAGS) -I../../common/amlsysctl -I../../common/amstreaminfo
libgstamlasink_la_LIBADD = $(GST_LIBS)
libgstamlasink_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)
libgstamlasink_la_LIBADD += ../../common/libcommon.a
libgstamlasink_la_LIBTOOLFLAGS = --tag=disable-static

# headers we need but don't want installed
noinst_HEADERS = gstamlasink.h amlasink_prop.h