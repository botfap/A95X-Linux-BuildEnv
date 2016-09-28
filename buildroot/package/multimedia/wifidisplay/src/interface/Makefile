TOPDIR := ..
SRCDIR := .
TARGET := libmiracast.so
CPPFLAGS += -fPIC -shared

-include $(TOPDIR)/Makefile.rule

.PHONY : all clean install

all: $(TARGET)

clean:
	@$(RM_F) $(PREFIX)/$(TARGET)
	@ echo CLEAN DONE


install: $(TARGET)
	-install -m 755 ${TARGET} $(STAGING)/
	cp -dp $(STAGING)/${TARGET} $(PREFIX)/

install_include:
	cp -rf $(TOPDIR)/include/interface $(STAGING_INC)/
	cp -rf $(STAGING_INC)/interface $(PREFIX_INC)/

$(TARGET):
	@ echo MAKE $(TARGET) START
	@ $(CXX) $(SOURCE) $(CPPFLAGS) -o $(TARGET)
	@ echo MAKE $(TARGET) DONE
