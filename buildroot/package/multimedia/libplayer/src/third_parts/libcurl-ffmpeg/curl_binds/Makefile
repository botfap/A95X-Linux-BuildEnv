M_PREFIX=$(LIBPLAYER_STAGING_DIR)/usr
ROOTFS?=$(M_PREFIX)
LIB_DIR?=$(M_PREFIX)/lib
HEADERS_DIR?=$(M_PREFIX)/include
LIBMOD_DIR=$(TARGET_DIR)/system/lib/amplayer
TARGET_OBJS=curl_ffmpeg.o curl_mod.o 

TARGET=libcurl_mod.so
INSTALL_DIR=${LIB_DIR}/libplayer/

TOPDIR=$(shell pwd)
SRC?=$(TOPDIR)
CFLAGS+=-O2 -fPIC
CFLAGS+=-I$(STAGING_DIR)/usr/include/curl -I$(SRC)/../include -I$(SRC)/../../../amffmpeg/  -I$(SRC)/../../../amffmpeg/$(wildcard libavformat/*.h) -I$(SRC)/../../../amavutils/include 
LDFLAGS +=-L$(M_PREFIX)/lib/libplayer -lavutil -lavformat -lavcodec -lm -lz -lpthread -lamcodec -ldl -lamplayer -lamadec -lasound -lamstreaming  -lamavutils -lrt -lstdc++ -lcurl 

export CC CFLAGS  LIBMOD_DIR

all: $(TARGET) 
	$(MAKE) install

$(TARGET): $(TARGET_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(TARGET_OBJS:%.o=%.c) -shared -o $@ -Wl,--whole-archive -L$(M_PREFIX)/lib/libplayer -lcurl_common -lcurl_base   -Wl,--no-whole-archive   

install:
	mkdir -p $(LIBMOD_DIR)
	-install -m 555 ${TARGET} $(LIBMOD_DIR)

force:

clean:
	-rm -rf *.o 

