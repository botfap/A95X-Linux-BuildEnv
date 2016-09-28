TOPDIR := ..
SRCDIR := .
TARGET := libwfd.so
CPPFLAGS += -fPIC -shared
CPPFLAGS += -I$(WFD_INC_DIR)

-include $(TOPDIR)/Makefile.rule

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
	@ $(CXX)  $(SOURCE) $(CPPFLAGS) -o $(TARGET)
	@ echo MAKE $(TARGET) DONE 
