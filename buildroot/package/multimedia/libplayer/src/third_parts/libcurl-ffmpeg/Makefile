#CC=${HOST_GCC}

#export CC BUILD_DIR STAGING_DIR TARGET_DIR
all:
	mkdir -p $(shell pwd)/../../staging/usr/lib/libcurl_ffmpeg
	-$(MAKE) -C curl_common all
	-$(MAKE) -C curl_base all
	-$(MAKE) -C curl_binds all
	
install:


clean:
	-$(MAKE) -C curl_binds clean
	-$(MAKE) -C curl_common clean
	-$(MAKE) -C cur_base  clean


