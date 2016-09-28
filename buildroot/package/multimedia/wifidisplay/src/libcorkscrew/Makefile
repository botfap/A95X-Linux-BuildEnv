MAKE = make
TOPDIR := ..
SRCDIR := .
TARGET := libcorkscrew.so
CC = arm-none-linux-gnueabi-gcc
CFLAGS += -I$(TOPDIR)/include
CFLAGS += -fPIC -shared
RM_F := rm

CFLAGS += -Wall
DEBUG = y

ifeq ($(DEBUG), y)
CFLAGS += -g
else
CFLAGS += -O2
endif

SOURCE := $(wildcard $(SRCDIR)/*.c) $(wildcard $(SRCDIR)/*.cpp)
OBJS := $(patsubst %.c,%.o,$(patsubst %.cpp,%.o,$(SOURCE)))
.PHONY : all clean install

all : $(TARGET)

clean :
	@ $(RM_F) $(PREFIX)/$(TARGET)
	@ echo CLEAN DONE

install:$(TARGET)
	-install -m 755 ${TARGET} $(STAGING)/
	cp -dp $(STAGING)/${TARGET} $(PREFIX)/

$(TARGET) :
	@ echo MAKE $(TARGET) START
	@ $(CC) $(SOURCE) $(CFLAGS) -o $(TARGET)
	@ echo MAKE $(TARGET) DONE 
