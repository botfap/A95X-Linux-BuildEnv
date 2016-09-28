UTILS = wifi_power usbpower usbtestpm usbtestpm_mx usbtestpm_mx_iddq usbpower_mx_iddq
.PHONY: all install clean

wifi_power: wifi_power.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

usbpower: usbpower.cpp usbctrl.cpp
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

usbtestpm: usbtestpm.cpp usbctrl.cpp
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

usbtestpm_mx: usbtestpm_mx.cpp usbctrl.cpp
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

usbtestpm_mx_iddq: usbtestpm_mx_iddq.cpp usbctrl_mx_iddq.cpp
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

usbpower_mx_iddq: usbpower_mx_iddq.cpp usbctrl_mx_iddq.cpp
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

all: $(UTILS)


clean:
	rm -f *.o $(UTILS)

install:
	install -m 755 $(UTILS) $(TARGET_DIR)/usr/bin/

uninstall:
	rm -f $(TARGET_DIR)/usr/bin/
