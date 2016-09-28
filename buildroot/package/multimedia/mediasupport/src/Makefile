COMPILE = $(CXX)
TARGET = MediaSupport
SOURCES = MediaSupport.cpp
CFLAGS = -O2 -I.
LDFLAGS = -L$(STAGING_DIR)/usr/lib -L$(STAGING_DIR)/usr/lib/libplayer -lamplayer -lavutil -lavformat -lavcodec -lm -lz -lpthread -lamcodec -lamadec -lasound -lamstreaming -lamavutils -lYCTVMedia -lYCTVOEM 

OBJECTS = $(SOURCES:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(COMPILE) $(OBJECTS) $(LDFLAGS) -o $@

.cpp.o:
	$(COMPILE) $(CFLAGS) $< -c -o $@

clean:
	-rm -f $(OBJECTS) $(TARGET)

install:
	cp -f $(TARGET) $(TARGET_DIR)/usr/bin/

uninstall:
	-rm -f $(TARGET_DIR)/usr/bin/$(TARGET)
