#!/bin/sh
SET_PATH=$( echo "$PATH" | grep elf32-em4)
if [  "$SET_PATH" = "" ]; then
ARC_EM4_PREFIX=../toolchains/opt/arc-4.8-amlogic-20130904-r2
PATH=$PATH:${ARC_EM4_PREFIX}/bin
fi

SET_PATH=$( echo "$PATH" | grep arm-none)
if [  "$SET_PATH" = "" ]; then
ARM_PREFIX=../toolchains/opt/CodeSourcery/Sourcery_G++_Lite
PATH=$PATH:${ARM_PREFIX}/bin:${ARM_PREFIX}/arm-none-eabi/bin:${ARM_PREFIX}/arm-none-linux-gnueabi/bin
fi

SETGNUEABIHF_PATH=$( echo "$PATH" | grep arm-linux-gnueabihf)
if [  "$SETGNUEABIHF_PATH" = "" ]; then
ARMHF_PREFIX=../toolchains/opt/gcc-linaro-arm-linux-gnueabihf
PATH=$PATH:${ARMHF_PREFIX}/bin
fi

AARCH64_LINUX_GNU_PATH=$( echo "$PATH" | grep aarch64-linux-gnu-gcc)
if [  "$AARCH64_LINUX_GNU_PATH" = "" ]; then
ARC64_LINUX_PREFIX=../toolchains/opt/gcc-linaro-aarch64-linux-gnu-4.9-2014.09_linux
PATH=$PATH:${ARC64_LINUX_PREFIX}/bin
fi

AARCH64_NONE_ELF_PATH=$( echo "$PATH" | grep aarch64-none-elf-gcc)
if [  "$AARCH64_NONE_ELF_PATH" = "" ]; then
AARCH64_NONE_ELF_PREFIX=../toolchains/opt/gcc-linaro-aarch64-none-elf-4.8-2013.11_linux
PATH=$PATH:${AARCH64_NONE_ELF_PREFIX}/bin
fi
