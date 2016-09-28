COMPILE = $(CXX)
TARGET = AudioCapture
SOURCES = AudioCapture.cpp amAudio.cpp
CFLAGS = -O2 -I. -I.
LDFLAGS = -L. -L$(STAGING_DIR)/usr/lib -lYCTVHACS -lYCTVOEM -lasound -lpthread

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
