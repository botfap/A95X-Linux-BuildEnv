#!/bin/bash
echo "### Adding i386 arch for cross compile toolchais ###"
sudo dpkg --add-architecture i386
echo "### Updating package list                        ###"
sudo apt-get update > /dev/null
echo "### Installig X64 packages                       ###"
sudo apt-get install -y p7zip-full p7zip-rar squashfs-tools micro-httpd subversion zip unzip git tree libncurses5-dev bzip2 lib32z1 lib32gcc1 dialog build-essential
echo "### Installing i386 dependencies                 ###"
sudo apt-get install -y libc6:i386 libncurses5:i386 libstdc++6:i386
echo "### Complete. All package installed with sucsess ###"
