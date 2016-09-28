# Note: plugindir is set in configure

##############################################################################
# TODO: change libgstplugin.la to something else, e.g. libmysomething.la     #
##############################################################################
plugin_LTLIBRARIES = libgstvout.la

##############################################################################
# TODO: for the next set of variables, name the prefix if you named the .la, #
#  e.g. libmysomething.la => libmysomething_la_SOURCES                       #
#                            libmysomething_la_CFLAGS                        #
#                            libmysomething_la_LIBADD                        #
#                            libmysomething_la_LDFLAGS                       #
##############################################################################

# sources used to compile this plug-in
libgstvout_la_SOURCES = gstamlvout.c gstamlvideoheader.c gstamlvideoheader.h gstamlvout.h

# compiler and linker flags used to compile this plugin, set in configure.ac
libgstvout_la_CFLAGS = $(GST_CFLAGS)
libgstvout_la_LIBADD = $(GST_LIBS) 
libgstvout_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)
libgstvout_la_LIBTOOLFLAGS = --tag=disable-static

# headers we need but don't want installed
noinst_HEADERS = gstamlvout.h gstamlvideoheader.h
