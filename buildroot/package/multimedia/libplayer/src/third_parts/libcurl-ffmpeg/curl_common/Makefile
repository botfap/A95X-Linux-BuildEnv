M_PREFIX=$(LIBPLAYER_STAGING_DIR)/usr
ROOTFS?=$(M_PREFIX)
LIB_DIR?=$(M_PREFIX)/lib
HEADERS_DIR?=$(M_PREFIX)/include

TARGET_OBJS=curl_common.o curl_fifo.o curl_list.o 

TARGET=libcurl_common.a
INSTALL_DIR=${LIB_DIR}/libplayer/

TOPDIR=$(shell pwd)
SRC?=$(TOPDIR)

CFLAGS+=-O2 -fPIC
CFLAGS+= -I$(SRC)/../include -I$(SRC)/../../../amffmpeg/ -I$(SRC)/../../../amffmpeg/libavutil/ 
LDFLAGS+= -L$(PREFIX)/lib/libplayer -lavutil -lavformat -lavcodec -lm -lz -lpthread -lamcodec -ldl -lamplayer -lamadec -lasound -lamstreaming  -lamavutils -lrt -lstdc++ -lcurl 
AR =$(STAGING_DIR)/../../bin/arm-linux-gnueabihf-ar
export CC CFLAGS
all: $(TARGET) 
	$(MAKE) install
	
$(TARGET): $(TARGET_OBJS)
	$(AR) cq $(TARGET) $(TARGET_OBJS)
	
$(TARGET_OBJS):%.o:%.c
	$(CC) $(CFLAGS) $(LDFLAGS) -c -o $@ $<
	
install:
	-install -m 555 ${TARGET} $(INSTALL_DIR)
	cp	-rf $(SRC)/../include/*.h  $(HEADERS_DIR)/

force:

clean:
	-rm -rf *.o 

