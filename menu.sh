#!/bin/bash

#Check Host OS & permissions
BPERM=`whoami`
BDIST=`cat /etc/lsb-release | grep DISTRIB_ID | cut -d "=" -f 2`
BREL=`cat /etc/lsb-release | grep DISTRIB_RELEASE | cut -d "=" -f 2`
BARCH=`hostnamectl | grep Architecture | cut -d ":" -f 2 | sed 's/^[[:space:]]*//'`

source buildroot/setup-build-env.sh

echo "#############################################"
echo "### Checking system type and permissions: ###"
echo "#############################################"
echo ""

if [ $BPERM = "root" ]; then
	echo "You must run the menu & build system as a normal user. NOT ROOT"
	exit
fi

if [[ $BDIST = "Ubuntu" && $BARCH = "x86_64" ]] && [[ $BREL = "14.04" || $BREL = "16.04" ]]; then
	misc-build-scripts/menu-main.sh $BPERM $BDIST $BREL $BARCH
else
	dialog --title 'Error' --msgbox 'You are not running Ubuntu X64 LTS. Please update your system or run the build system manually if your sure you wont break something.' 10 40
	exit
fi

