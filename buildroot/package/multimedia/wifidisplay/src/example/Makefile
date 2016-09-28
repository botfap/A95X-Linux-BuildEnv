TOPDIR := ..
SRCDIR := .
TARGET := wfd

LDFLAGS := -L$(TARGET_DIR)/usr/lib -lz
LDFLAGS += -lavutil -lavformat -lavcodec -lamcodec \
				-lamplayer -lamadec -lasound -lamstreaming -lamavutils
LDFLAGS += -lwfd -lwifiplayer -lfound -lutils -lstagefright -lcorkscrew 
LDFLAGS += -L$(TARGET_DIR)/lib -lm -ldl -lrt -lpthread

-include $(TOPDIR)/Makefile.rule

.PHONY : all clean

all : $(TARGET)

clean:
	@ echo CLEAN DONE

install:
	-install -m 777 ${TARGET} $(INSTALL_STAG)
	cp -dp $(INSTALL_STAG)/${TARGET} $(INSTALL_PREFIX)
	
$(TARGET): $(OBJS)
	@ echo MAKE $(TARGET) START
	$(CXX) $(CPPFLAGS) $(LDFLAGS) $(SOURCE) -o $(TARGET)
	@ echo MAKE $(TARGET) DONE

