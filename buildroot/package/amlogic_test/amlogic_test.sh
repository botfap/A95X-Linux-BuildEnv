#!/bin/sh

echo " "
echo "============ Amlogic test ============"
echo " "

#rebootfile=""
#if [ ! -f "$rebootfile" ]; then
# if [ ! -e "/lib/preinit" ]; then
#    echo "no /lib/preinit"
#    mkdir /lib/preinit
# fi
# cp /bin/auto_reboot.sh /lib/preinit
# cp /bin/01_preinit.sh /lib/preinit
#fi

echo "Please select the item you want to test:"
echo "-----------------------------------------"
echo "1. Auto scaling test";
echo "2. Auto reboot test";
echo "3. Auto suspend test";
echo "4. DDR stress test";
echo "5. NAND test";
echo "6. WiFi test";
echo "7. BT test";
echo "-----------------------------------------"
echo " "
itemno=0
read itemno
case $itemno in
1) echo "Auto scaling test";
	auto_freq.sh;;
#	repeat=3000 auto_freq_openwrt.sh;;
2) echo "Auto reboot test";
        if [ ! -e "/lib/preinit" ]; then
	    echo "no /lib/preinit"
	    mkdir /lib/preinit
	fi
	cp /usr/bin/auto_reboot.sh /lib/preinit
	#cp /usr/bin/01_preinit.sh /lib/preinit
	fcnt=/lib/preinit/cnt;                 
	if [ -e "$fcnt" ]; then
	    rm -f $fcnt;                   
	fi
	reboot;;
3) echo "Auto suspend test";
	auto_suspend 0 20 10 10;;	
4) echo "DDR pressure test";
	stressapptest -s 604800 -W -l /tmp/ddrstress.txt;;
5) echo "NAND test";
	disk-test.sh;;
6) echo "WiFi test";
	wifi_test.sh;;
7) echo "BT test";
	bt.sh;;
*) echo "Invalid value !!!";;
esac

exit 0
