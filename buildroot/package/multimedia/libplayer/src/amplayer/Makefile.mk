PREFIX=$(LIBPLAYER_STAGING_DIR)/usr
ROOTFS?=$(PREFIX)

CROSS=$(CC:%gcc=%)

QUITE_CMD?=no

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

ifeq ($(QUITE_CMD),yes)
Q=@
endif   
export Q

TARGET?=libamplayer.so

TARGET_IS_LIB=$(TARGET:%so=yes)

LDFLAGS+= -L$(INSTALL_DIR) -lavutil -lavformat -lavcodec -lm  -lpthread -L$(STAGING_DIR)/usr/lib/libplayer -lamavutils

INSTALL_DIR?=$(PREFIX)/lib/libplayer
LDFLAGS+=-shared 
CFLAGS=$(DIRS:%/=-I$(SRC)/%/include) 

ifeq ($(TARGET),libamplayer.so)
    DIRS=player/
else
    DIRS=control/
    CFLAGS+=-I$(SRC)/player/include
endif


CFLAGS+= -I${SRCTREE}/../amffmpeg -I${SRCTREE}/../amcodec/include -I${SRCTREE}/../amadec/include
CFLAGS+= -fPIC -g
target_all=  $(TARGET)



INCLUDE=${SRCTREE}/include/

CFLAGS+=-Wall 
#CFLAGS+=-Werror

CFLAGS+=-O0  -gdwarf-2  -g
#CFLAGS+=-O2

ifdef DOLBY_DAP
LOCAL_CFLAGS += -DDOLBY_DAP_EN
endif

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
	-install  $(INSTALL_FLAGS)  $(target_all)  $(INSTALL_DIR)
	-install  $(INSTALL_FLAGS) $(DIRS:%/=$(SRC)/%/include/*)  $(PREFIX)/include
	cp -rf player/include/sys  $(PREFIX)/include
    

