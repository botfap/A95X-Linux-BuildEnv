CROSS=$(CC:%gcc=%)

esplayer_s=esplayer.c

INSTALL_FLAGS=-m 755

TARGET=esplayer

LDFLAGS= -L$(STAGING_DIR)/usr/lib/aml_libs -lamcodec -ldl -lamadec -lasound -lamavutils
#CFLAGS+= -I${BUILD_DIR}/../amcodec/include -I${BUILD_DIR}/../amadec/include -pthread
CFLAGS+= -I$(shell pwd)/../amcodec/include -I$(shell pwd)/../amadec/include  -I$(shell pwd) -pthread
CFLAGS+=-O2 -g

#ifeq ($(BR2_TARGET_BOARD_PLATFORM),meson6)
    FIRMWARE_TYPE=firmware-m6
#else ifeq ($(BR2_TARGET_BOARD_PLATFORM),meson8)
#    FIRMWARE_TYPE=firmware-m8
#else
#    FIRMWARE_TYPE=firmware
#endif

.PHONY: all config  before_cmd install clean distclean

all: ${TARGET}
	mkdir -p ${PREFIX}/lib/firmware

install:all
	install $(INSTALL_FLAGS) $(TARGET) $(TARGET_DIR)/bin
#	cp -rf ${BUILD_DIR}/../amadec/$(FIRMWARE_TYPE)/*.bin ${PREFIX}/lib/firmware
distclean:clean
    
clean:
	rm $(TARGET_DIR)/bin/$(TARGET)

${TARGET}:esplayer.o
	$(CC) $(CFLAGS) $(LDFLAGS) $($(@:%=%_s)) -o $@
