# Backup & Recovery of the A95X rev2 with TWRP
** Do not use this for S905 based devices, you will set them on fire! **


Before you can even think about recovering that lovely little device you just bricked you are going to need something to restore it from. So before you start breaking things lets make a backup to reduce the tears later.

We will be using TWRP 3.0.2 for both backup and recovery as it can be booted from USB, uSD and eMMC and makes backup and restore easy.

# TWRP Recovery for S905X only
** Do not use this for S905 based devices, you will set them on fire! **


	1) Format a uSD or USB flash with a FAT32 file system

	2) Copy "twrp-s905x.img" to the root of your freshly formatted flash device

	3) Rename "twrp-s905x.img" to "recovery.img"

	4) Boot A95X while holding in the reset/recovery button hidden inside the AV port

	5) You are now in the TWRP recovery environment.


Notes: You can also install this TWRP to the A95X as a replacement for the stock recovery. Jutst flash it as an image from TWRP to the recovery partition.


# Backup your stock install
We will create a TWRP flashable image of your system. Note that TWRP sees the 2GB devices as a different device to 1GB devices. If you have both 1GB and 2GB A95X devices you should take a backup of both types.


	1) Boot TWRP from your recovery flash while holding in the reset/recovery button hidden inside the AV port

	2) While booted into TWRP rec env click on the "Backup" button on the homescreen

	3) Click the "Select Storage" button and pick the storage device you want to hold the backup, probably whatever you are booting the recovery from.

	4) Select the Partitions you want to backup. Select all for a full device backup. Select only Boot & System if you want to create a factory reset image.

	5) Complete. Save that backup somewhere SAFE


# Recover your stock install


	1) Boot TWRP from your recovery flash while holding in the reset/recovery button hidden inside the AV port

	2) While booted into TWRP rec env click on the "Restore" button on the homescreen

	3) Click the "Select Storage" button and pick the storage device you want to restore the backup from, probably whatever you are booting the recovery from.

	4) Select the Partitions you want to restore. Select all for a full device restore. Select only Boot & System if you want to create a factory reset image.

	5) Complete. Reboot to your previous install

