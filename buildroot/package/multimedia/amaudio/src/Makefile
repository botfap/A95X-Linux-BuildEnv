DEPS = 
OBJ = amAudio.o

%.o: %.cpp $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

amaudio: $(OBJ)
	$(CXX) -lasound -lpthread -o $@ $^ $(CFLAGS)

.PHONY: clean

clean:
	rm -f *.o *~ amaudio

.PHONY: install

install:
	cp -f amaudio $(TARGET_DIR)/usr/bin/

.PHONY: uninstall

uninstall:
	rm -f $(TARGET_DIR)/usr/bin/amaudio
