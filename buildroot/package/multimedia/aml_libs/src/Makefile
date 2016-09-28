#CC=${HOST_GCC}
ifeq ($(BR2_TARGET_BOARD_PLATFORM),"meson8")
	FIRMDIR := firmware-m8
else 
ifeq ($(BR2_TARGET_BOARD_PLATFORM),"meson6")
	FIRMDIR := firmware-m6
else 
	FIRMDIR := firmware
endif
endif
#export CC BUILD_DIR STAGING_DIR TARGET_DIR
all:
	mkdir -p $(shell pwd)/../../staging/usr/lib/aml_libs
	mkdir -p $(shell pwd)/../../staging/usr/lib/libplayer
	-$(MAKE) -C amadec all
	-${MAKE} -C audio_codec all
	-$(MAKE) -C amavutils all
	-$(MAKE) -C amcodec all
	-$(MAKE) -C example all
	echo $(BR2_TARGET_BOARD_PLATFORM)
	echo $(FIRMDIR) 
install:
	-$(MAKE) -C amadec install
	-$(MAKE) -C amavutils install
	-$(MAKE) -C amcodec install
	-$(MAKE) -C example install
	-${MAKE} -C audio_codec install	
	cp  -rf $(AML_LIBS_STAGING_DIR)/usr/lib/aml_libs/*.so $(TARGET_DIR)/usr/lib 
	cp  -rf $(AML_LIBS_STAGING_DIR)/usr/lib/aml_libs/*.so $(AML_LIBS_STAGING_DIR)/usr/lib/libplayer 
	mkdir -p $(TARGET_DIR)/lib/firmware
	install -D -m 0644 amadec/${FIRMDIR}/*.bin $(TARGET_DIR)/lib/firmware 
	cp  -rf drm-libs/*.tzo $(TARGET_DIR)/lib/ 
	cp  -rf drm-libs/*.so $(TARGET_DIR)/lib/
	cp  -rf audio_codec/libeac3/libeac3.so $(TARGET_DIR)/usr/lib
clean:
	-$(MAKE) -C amadec clean
	-$(MAKE) -C amavutils clean
	-$(MAKE) -C amcodec clean
	-$(MAKE) -C example clean
	-${MAKE} -C audio_codec clean


