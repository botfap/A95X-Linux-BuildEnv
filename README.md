# A95X-Linux-BuildEnv

A95X (S905X Variant) Source code, toolchains and build system to build:

	Linux 3.14 Kernel Image - Image
	Boot Image with kernel & modules - boot.img
	System Device Tree - gxl_p212_2g.dtb, dtb.img
	RootFS Tarball - rootfs.tar.gz
	RootFS EXT2 Image for dd - rootfs.ext2.gz
	Uboot for eMMC boot - u-boot.bin
	Uboot for SD boot - u-boot.bin.sd.bin

#Install Toolchains
	mv -Rv toolchains/opt/* /opt/

#Setup build
Change into the buildroot folder

Setup build env vars
	source TOOLSENV.sh

Generate config for your device
For A95X-2G:		make s905x_a95x_2g_defconfig
For A95X-1G:		make s905x_a95x_1g_defconfig
For Generic S905X:	make mesongxl_p212_release_defconfig

#Build output
Just fire off make, make brew and sunday dinner
Use -jX for multi threaded compiling to icrease speed, X=number of cpu cores or hw threads available for max performance
e.g.
	make
	make -j4

Important output will be in output/images
	botfap@devbox:~/s905x$ ls -la buildroot/output/images/
	total 314224
	drwxr-xr-x 2 botfap botfap     4096 Sep 28 07:10 .
	drwxr-xr-x 6 botfap botfap     4096 Sep 28 04:38 ..
	-rw-r--r-- 1 botfap botfap 20408441 Sep 28 07:04 boot.img
	lrwxrwxrwx 1 botfap botfap       15 Sep 28 07:10 dtb.img -> gxl_p212_2g.dtb
	-rw-r--r-- 1 botfap botfap    37278 Sep 28 05:49 gxl_p212_2g.dtb
	-rw-r--r-- 1 botfap botfap 16973552 Sep 28 05:49 Image
	-rw-rw-r-- 1 botfap botfap       22 Sep 28 06:15 README.md
	-rw-r--r-- 1 botfap botfap  3391781 Sep 28 07:04 rootfs.cpio.gz
	-rw-r--r-- 1 botfap botfap  3391845 Sep 28 07:04 rootfs.cpio.uboot
	-rw-r--r-- 1 botfap botfap 93696435 Sep 28 05:49 rootfs.ext2.gz
	-rw-r--r-- 1 botfap botfap 91347941 Sep 28 07:05 rootfs.tar.gz
	-rw-r--r-- 1 botfap botfap 90646041 Sep 28 05:49 rootfs.tgz
	-rw-r--r-- 1 botfap botfap   917504 Sep 28 05:49 u-boot.bin
	-rw-r--r-- 1 botfap botfap   918016 Sep 28 05:49 u-boot.bin.sd.bin
