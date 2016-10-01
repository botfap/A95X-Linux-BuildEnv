#!/bin/bash

#Check Host OS & permissions
shopt -s dotglob
BPERM=`whoami`
BDIST=`cat /etc/lsb-release | grep DISTRIB_ID | cut -d "=" -f 2`
BREL=`cat /etc/lsb-release | grep DISTRIB_RELEASE | cut -d "=" -f 2`
BARCH=`hostnamectl | grep Architecture | cut -d ":" -f 2 | sed 's/^[[:space:]]*//'`

echo "#############################################"
echo "### Checking system type and permissions: ###"
echo "#############################################"
echo ""

if [ $BPERM != "root" ]; then
	echo "You must run the setup script as root or with sudo."
	exit
else
	echo "Permissions Check: OK - Running as root or with sudo"
	echo ""
	echo "Checking for Distro - Found: " $BDIST
	echo ""
	echo "Checking for Version - Found: " $BREL
	echo ""
	echo "Checking for Arch - Found: " $BARCH
	echo ""
fi

if [[ $BDIST = "Ubuntu" && $BARCH = "x86_64" ]] && [[ $BREL = "14.04" || $BREL = "16.04" ]]; then
	echo "#######################################################"
	echo "### All Checks passed, Installing required packages ###"
	echo "#######################################################"
	echo ""
	misc-build-scripts/inst-packages-ubuntu.sh
else
	echo "#############################################################################################"
	echo "### This build sytem is assembled for Ubuntu LTS. It supports both 14.04 & 16.04 x64 only ###"
	echo "#############################################################################################"
	exit
fi

	echo ""
	echo "############################################"
	echo "###      Installing toolchains           ###"
	echo "############################################"
	cp -Rv toolchains/opt/* /opt/ 
	source buildroot/setup-build-env.sh
	echo ""
	echo "############################################"
	echo "###      Toolchains Installeds           ###"
	echo "############################################"
