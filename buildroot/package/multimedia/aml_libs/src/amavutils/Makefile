M_PREFIX=$(AML_LIBS_STAGING_DIR)/usr
LIB_DIR?=$(M_PREFIX)/lib
HEADERS_DIR?=$(M_PREFIX)/include

TARGET_OBJS=amaudioutils.o amconfigutils.o amdisplayutils.o ammodule.o amsysfsutils.o Amvideoutils.o itemlist.o\

TARGET=libamavutils.so

INSTALL_DIR=${LIB_DIR}/aml_libs

LDFLAGS += -shared -lpthread -lm -lrt

TOPDIR=$(shell pwd)
SRC?=$(TOPDIR)

CFLAGS+=-O2 -fPIC -g
CFLAGS+=-I$(SRC) -I$(SRC)/include -I$(SRC)/../amcodec/include -L${LIB_DIR} -I$(HEADERS_DIR)





export CC CFLAGS

all: $(TARGET) 

$(TARGET): $(TARGET_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(TARGET_OBJS:%.o=%.c) -o $@  

    
    

install:
	-install -m 555 ${TARGET} $(INSTALL_DIR)
	cp  -rf $(SRC)/include/*  $(HEADERS_DIR)/

force:

clean:
	-rm -rf *.o ${AMADEC_DIST} ${MP3LIB} $(PCMLIB) $(ADPCMLIB) $(AACLIB) $(A52LIB)

    

