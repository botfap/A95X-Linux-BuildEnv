all:
	-make -C libmad all	
	-make -C libadpcm all
	-make -C libamr all
	-make -C libape all
	-make -C libcook all
	-make -C libfaad all
	-make -C libflac all
	-make -C liblpcm all
	-make -C libraac all
	-make -C libpcm all
	-make -C libamffmpegdec all
install:
	-make -C libmad install
	-make -C libadpcm install
	-make -C libamr install
	-make -C libape install
	-make -C libcook install
	-make -C libfaad install
	-make -C libflac install
	-make -C liblpcm install
	-make -C libraac install
	-make -C libpcm install
	-make -C libamffmpegdec install
	cp  -rf  libeac3/libdcv.so $(TARGET_DIR)/usr/lib
	cp  -rf  libdtscore/libdtscore.so $(TARGET_DIR)/usr/lib
clean:
	-make -C libmad clean
	-make -C libadpcm clean
	-make -C libamr clean
	-make -C libape clean
	-make -C libcook clean
	-make -C libfaad clean
	-make -C libflac clean
	-make -C liblpcm clean
	-make -C libraac clean
	-make -C libpcm clean
	-make -C libamffmpegdec clean
	-make -C libdtscore clean
	rm  -rf  $(TARGET_DIR)/usr/lib/libdcv.so
	rm  -rf  $(TARGET_DIR)/usr/lib/libdtscore.so
