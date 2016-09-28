################################################################################
#
# libcares
#
################################################################################

LIBCARES_VERSION_MAJOR = 1.10
LIBCARES_VERSION_MINOR = 0
LIBCARES_VERSION = $(LIBCARES_VERSION_MAJOR).$(LIBCARES_VERSION_MINOR)
LIBCARES_SOURCE = c-ares-$(LIBCARES_VERSION).tar.gz
LIBCARES_SITE = http://c-ares.haxx.se/download
LIBCARES_AUTORECONF = YES
LIBCARES_INSTALL_STAGING = YES

LIBCARES_CONF_OPTS = CFLAGS="-pipe -Os"

$(eval $(autotools-package))
