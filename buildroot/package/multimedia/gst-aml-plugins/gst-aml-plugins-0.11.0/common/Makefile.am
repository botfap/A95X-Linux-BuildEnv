# Note: plugindir is set in configure

##############################################################################
# TODO: change libgstplugin.la to something else, e.g. libmysomething.la     #
##############################################################################
lib_LIBRARIES = libcommon.a

##############################################################################
# TODO: for the next set of variables, name the prefix if you named the .la, #
#  e.g. libmysomething.la => libmysomething_la_SOURCES                       #
#                            libmysomething_la_CFLAGS                        #
#                            libmysomething_la_LIBADD                        #
#                            libmysomething_la_LDFLAGS                       #
##############################################################################

# sources used to compile this plug-in
libcommon_a_SOURCES = amlsysctl/gstamlsysctl.c amlsysctl/gstamlsysctl.h amstreaminfo/amlstreaminfo.c amstreaminfo/amlstreaminfo.h

# compiler and linker flags used to compile this plugin, set in configure.ac
libcommon_a_CFLAGS = $(GST_CFLAGS) -fPIC
noinst_HEADERS = amlsysctl/gstamlsysctl.h amstreaminfo/amlstreaminfo.h
