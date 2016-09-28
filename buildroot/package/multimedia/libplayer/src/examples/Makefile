PREFIX=$(LIBPLAYER_STAGING_DIR)/usr
TARGET_DIR=$(LIBPLAYER_TARGET_DIR)/usr
TARGET_FIRWARE_DIR=$(LIBPLAYER_TARGET_DIR)
LIB_DIR=$(PREFIX)/lib/libplayer
BUILD_DIR=$(shell pwd)
INC_DIR=$(LIBPLAYER_STAGING_DIR)/usr/include
CROSS=$(CC:%gcc=%)

kplayer_s=kplayer.c

INSTALL_FLAGS=-m 755

TARGET=kplayer

LDFLAGS= -L$(PREFIX)/lib/libplayer -lavutil -lavformat -lavcodec -lm -lz -lpthread -lamcodec -ldl -lamplayer -lamadec -lasound -lamstreaming  -lamavutils -lrt -lstdc++
CFLAGS+= -I${BUILD_DIR}/../amffmpeg -I${BUILD_DIR}/../amcodec/include -I${BUILD_DIR}/../amadec/include 
CFLAGS+= -I${BUILD_DIR}/../amplayer/player/include -I${BUILD_DIR}/../amplayer/player -I${BUILD_DIR}/../streamsource
CFLAGS+= -I$(INC_DIR)
CFLAGS+=-O2 -g


ifeq ($(BR2_TARGET_BOARD_PLATFORM),"meson6")
    FIRMWARE_TYPE=firmware-m6
else ifeq ($(BR2_TARGET_BOARD_PLATFORM),"meson8")
    FIRMWARE_TYPE=firmware-m8
else
    FIRMWARE_TYPE=firmware
endif

.PHONY: all config  before_cmd install clean distclean

all: ${TARGET}
	mkdir -p $(TARGET_DIR)/../lib/firmware
install:all
	install $(INSTALL_FLAGS) $(TARGET) $(TARGET_DIR)/bin
	cp -rf $(LIB_DIR)/*.so* $(TARGET_DIR)/lib
      
distclean:clean
    
clean:
	rm $(TARGET_DIR)/bin/$(TARGET)

${TARGET}:kplayer.o
	$(CC) $(CFLAGS) $(LDFLAGS) $($(@:%=%_s)) -o $@
