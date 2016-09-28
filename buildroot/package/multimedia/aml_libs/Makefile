
FILE_DIR=aml_libs

DEPENDS=zlib-1.2.3 alsa-lib-1.0.20

##DBUS_ON=DBUS

FFMPEG_CONFIG_FLAGS=--prefix=${PREFIX} --disable-static --enable-shared \
	--disable-ffmpeg --disable-ffplay --disable-ffserver --disable-doc \
    --disable-encoders --disable-decoder=h264 --disable-muxers --disable-filters --enable-cross-compile \
    --cross-prefix=arc-linux-uclibc- --extra-ldflags=-L${PREFIX}/lib --arch=arc --disable-altivec --disable-amd3dnow \
    --disable-amd3dnowext --disable-mmx --disable-mmx2 --disable-sse --disable-ssse3 --disable-armv5te --disable-armv6 \
    --disable-armv6t2 --disable-armvfp --disable-iwmmxt --disable-mmi --disable-neon --disable-vis --disable-yasm --enable-pic \
	--disable-avdevice --disable-v4l

    
CONFIG_FLAGS=

TAR_DIR=${PKG_BUILD_DIR}/${FILE_DIR}

P_FILE_F=${PKG_DIR}/${P_FILE}
BUILD_DIR=${PKG_BUILD_DIR}/${FILE_DIR}

.PHONY:configure	all config  before_cmd install clean distclean

QUITE_CMD=yes
#export QUITE_CMD
all:before_cmd config  	
	cp -a ${BUILD_DIR}/amadec/firmware ${PREFIX}/lib/
	cp -rf ${BUILD_DIR}/drm-libs/* ${PREFIX}/lib/
	$(MAKE) -C ${BUILD_DIR} all
	$(MAKE) -C ${BUILD_DIR} install
	
configure:
        cd ${BUILD_DIR}/${AMFFMPEG_DIR} && ${PKG_TOP_DIR}/${FILE_DIR}/src/${AMFFMPEG_DIR}/configure  ${FFMPEG_CONFIG_FLAGS}

install:all

distclean:clean
	
clean:
	-$(MAKE) -C ${BUILD_DIR} CC=${HOST_GCC} clean


before_cmd:depends

depends:
	@if [   "${DEPENDS}" != "" ]; then \
                cd ${PKG_TOP_DIR};$(MAKE) lib_depends TOP_DEPENDS="${DEPENDS}";    \
        fi
#include $(MAKEFILES_DIR)/package.rules
