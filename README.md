# A95X-Linux-BuildEnv

A95X (S905X Variant) Source code, toolchains and build system to build:

	Linux 3.14 Kernel Image - Image
	Boot Image with kernel & modules - boot.img
	System Device Tree - gxl_p212_2g.dtb, dtb.img
	RootFS Tarball - rootfs.tar.gz
	RootFS EXT2 Image for dd - rootfs.ext2.gz
	Uboot for eMMC boot - u-boot.bin
	Uboot for SD boot - u-boot.bin.sd.bin

# Compatability
Ive only tested onn Ubuntu LTS 14 & 16. If you can give me feedback on other distros I can make changes to make it work with yours normally. Only real requirements are for x64 host with i386 libs available.

# Install Packages & Toolchains
	See Readme in toolchains folder for toolchains
	
	OR
	
	Run the setup.sh script to configure your build environment

# Setup build
Change into the buildroot folder

Setup build env vars

	source setup-build-env.sh


Generate config for your device

For A95X-2G:		make s905x_a95x_2g_defconfig

For A95X-1G:		make s905x_a95x_1g_defconfig

For Generic S905X:	make mesongxl_p212_release_defconfig

To nose about:		make menuconfig

# Build output
Just fire off make, make brew and sunday dinner

Use -jX for multi threaded compiling to icrease speed, X=number of cpu cores or hw threads available for max performance

	make

	make -j4


Important output will be in output/images

	botfap@devbox:~/s905x$ ls -la buildroot/output/images/

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

# Install to SD - Generic dev s905x
1. Create an SD card with one partition in ext2 format.

2. Copy boot.img, rootfs.tar.gz to this partition

        sudo cp output/images/boot.img /media/sdcard

        sudo cp output/images/rootfs.tar.gz /media/sdcard

        sudo sync
	
3. Extract rootfs.tar.gz on SD card

        cd /media/sdcard
	
        sudo tar zxvf rootfs.tar.gz
	
        sync

4. Write uboot to SD card

        sudo dd if=output/images/u-boot.bin.sd.bin of=/dev/mmcblkX bs=1 count=442

        sudo dd if=output/images/u-boot.bin.sd.bin of=/dev/mmcblkX bs=512 skip=1 seek=1

        sudo sync

5. Access UBoot
You'll need to access the serial console and enter into the UBoot prompt.
On the Nexbox A95x S905x, there are 4 exposed test points next to the USB ports. The "top" pin (in reference to the text on the case and marked by the arrow on the board) is ground, below that is RX, followed by TX. The console runs at 115200 baud and you have to hold down enter while it's booting to enter the UBoot console. 

6. When running into uboot, execute `run bootsdcard` under the prompt:

        env default -a
	
        env save

        run bootsdcard

7. (optional) To make this the default behavior (be careful, especially if you're changing the EMMC bootloader!), you can run the following commands at UBoot instead:

        env default -a
	
        setenv bootcmd "run bootsdcard"
	
        env save
	
        run bootsdcard

# Install to eMMC - Generic dev s905x
Warning! 	This will fuckup whatever is on the device, literally set that little bad boy on fire. DO NOT DO THIS

** Before going any further go into the recovery folder and read the Readme.md **


1. Create an SD card with one partition in vfat format

2. copy boot.img and root file system to SD card

        cp output/images/u-boot.bin /media/mySD

        cp output/images/boot.img /media/mySD

        cp output/images/rootfs.tar.gz /media/mySD 

Insert SD card into your platform and reboot into uboot. Replace original uboot with the new one under uboot prompt:

	mmcinfo

	fatload mmc 0 ${loadaddr} u-boot.bin

	store rom_write	${loadaddr} 0 120000

	fatload mmc 0 ${loadaddr} gxl_p212_2g.dtb

	store dtb write	${loadaddr}

	reset

3. With new uboot burned on your platform, enter uboot prompt again and execute `run bootupdate`

        env default -a

        env save

        run bootupdate

4. System will automatically write kernel to boot partition and extract rootfs.tar.gz to system partition.

5. Reboot platform.

6. System will boot up with kernel and root filesystem on EMMC/NAND.



# Supported devices in current master

S905:
	Not Yet

S905X:
	s905x_a95x_1g_defconfig - Nexbox A95X rev2 1GB/8GB
	s905x_a95x_2g_defconfig - Nexbox A95X rev2 2GB/16GB
	mesongxl_p212_release_defconfig - Generix S905X build

S912:
	Not Yet
