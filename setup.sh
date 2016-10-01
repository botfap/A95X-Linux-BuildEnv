#!/bin/bash

#Check Host OS
BDIST=`cat /etc/lsb-release | grep DISTRIB_ID | cut -d "=" -f 2`
BREL=`cat /etc/lsb-release | grep DISTRIB_RELEASE | cut -d "=" -f 2`
BARCH=`hostnamectl | grep Architecture | cut -d ":" -f 2 | sed 's/^[[:space:]]*//'`
