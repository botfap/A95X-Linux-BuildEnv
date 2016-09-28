PREFIX=$(LIBPLAYER_STAGING_DIR)/usr

LIB_DIR?=$(PREFIX)/lib
HEADERS_DIR?=$(PREFIX)/include
CROSS=$(CC:%gcc=%)
QUITE_CMD=1

INSTALL_FLAGS=-m 755

CPP=$(CROSS)g++
AS=$(CROSS)as
AR=$(CROSS)ar
LD=$(CROSS)ld
NM=$(CROSS)nm
STRIP=$(CROSS)strip
OBJCOPY=$(CROSS)objcopy
export CC CPP AS AR LD NM STRIP OBJCOPY

TOPDIR=$(shell pwd)
SRC?=$(TOPDIR)
SRCTREE=$(SRC)
BUILDTREE=$(SRC)/out

INSTALL_DIR=${LIB_DIR}/libplayer

TARGET?=libamcodec.so

DIRS=audio_ctl/ codec/

LDFLAGS=  -O2  -L$(LIB_DIR)  -lm -lc  -shared -Wl,--shared -Wl,-soname,$(TARGET)$(LIB_VERSION)

#INCLUDE=$(DIRS:%/=-I$(SRC)/%/include)
INCLUDE=${SRCTREE}/include/ 
#CFLAGS=-O0  -gdwarf-2  -g
CFLAGS=-O2 -g  -I$(SRC)/include  -I${SRCTREE}/audio_ctl -I${SRCTREE}/codec -I${SRCTREE}/../amadec/include -I${SRCTREE}/codec/yuvplayer -fPIC




target_all=  $(TARGET)
all:$(target_all)

dirs=$(DIRS)

DIROBJ=$(dirs:%/=%-dir)


$(target_all):$(DIROBJ)
	$(call ld_mk,$(OUT_DIR)$@,$^)




include $(SRCTREE)/rules.mk
export CC CPP AS AR LD TOPDIR SRCTREE  CFLAGS LDFLAGS Q

CLRDIR=$(dirs:%/=%-clrdir)
clean:$(CLRDIR)
	rm -rf $(target_all)

install:$(target_all)
	-install  -m 755   $(TARGET)  $(INSTALL_DIR)
	cp  -rf $(SRC)/include/*  $(HEADERS_DIR)/
    

