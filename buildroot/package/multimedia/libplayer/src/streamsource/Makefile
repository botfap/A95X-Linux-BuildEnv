M_PREFIX=$(LIBPLAYER_STAGING_DIR)
ROOTFS?=$(M_PREFIX)
LIB_DIR?=$(M_PREFIX)/usr/lib
HEADERS_DIR?=$(M_PREFIX)/usr/include

TARGET_OBJS=ffmpegsource.o queue.o source.o streambufqueue.o streamsource.o thread_read.o\

TARGET=libamstreaming.so

INSTALL_DIR=${LIB_DIR}/libplayer

LDFLAGS += -shared -lpthread -lm -lrt -L$(INSTALL_DIR) -lamplayer

TOPDIR=$(shell pwd)
SRC?=$(TOPDIR)

CFLAGS+=-O2 -fPIC
CFLAGS+=-I$(SRC) -I$(SRC)/../amffmpeg/ -I$(SRC)/../amavutils/include -I${M_PREFIX}/include -I$(HEADERS_DIR)


export CC CFLAGS

all: $(TARGET) 
	$(MAKE) install

$(TARGET): $(TARGET_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(TARGET_OBJS:%.o=%.c) -o $@  

	
	

install:
	-install -m 555 ${TARGET} $(INSTALL_DIR)
	cp	-rf $(SRC)/*.h  $(HEADERS_DIR)/

force:

clean:
	-rm -rf *.o ${AMADEC_DIST} ${MP3LIB} $(PCMLIB) $(ADPCMLIB) $(AACLIB) $(A52LIB)

	
