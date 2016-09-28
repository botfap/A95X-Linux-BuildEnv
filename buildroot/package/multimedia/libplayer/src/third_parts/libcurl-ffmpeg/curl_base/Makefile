M_PREFIX=$(LIBPLAYER_STAGING_DIR)/usr
ROOTFS?=$(M_PREFIX)
LIB_DIR?=$(M_PREFIX)/lib
HEADERS_DIR?=$(M_PREFIX)/include

TARGET_OBJS=curl_fetch.o curl_wrapper.o 

TARGET=libcurl_base.a
INSTALL_DIR=${LIB_DIR}/libplayer/

TOPDIR=$(shell pwd)
SRC?=$(TOPDIR)

CFLAGS+=-O2 -fPIC
CFLAGS+= -I$(SRC)/../include -I$(STAGING_DIR)/usr/include/curl
LDFLAGS+= -ldl -lstdc++ -lcurl 
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

force:

clean:
	-rm -rf *.o 

