#!/bin/sh

ETC_MODULE=/etc/modules
DRIVER=

# filter out mail module
WIFI_DRIVER=`awk '$1 != "mali" { print $0 }' $ETC_MODULE`

# get number of wifi drivers
NUMBER=`echo $WIFI_DRIVER | awk '{ print NF }'`

echo "wifi test start"
for i in $(seq 1 $NUMBER); do 
    DRIVER=`echo $WIFI_DRIVER | cut -d ' ' -f $i`
    echo $DRIVER 
    DRIVER_PATH=`find /lib/modules -name $DRIVER.ko`

    # if multiple .ko is found, get first one
    DRIVER_PATH=`echo $DRIVER_PATH | cut -d ' ' -f 1`
    echo $DRIVER_PATH
    if [ $DRIVER != "" ]; then
        rmmod $DRIVER
        sleep 1
	
        echo "insmod wifi driver"
	if [ $DRIVER == "dhd" ]; then
	    insmod $DRIVER_PATH firmware_path=/etc/wifi/fw_bcm40183b2.bin nvram_path=/etc/wifi/nvram.txt
	else
            insmod $DRIVER_PATH
	fi 
        sleep 1
    fi
done

echo "wifi up"
ifconfig wlan0 up 2> /tmp/wifi_error.log
sleep 1


#for j in "${array2[@]}";
#do 
#echo "%%%%%%"
#echo $j
#done
#echo $DRIVER_PATH
#insmod $DRIVER_PATH
#find . -name $i.ko
#echo "wifi test start"
#for i in $(seq 1 $1);do
#echo "insmod wifi driver"
#insmod /lib/modules/3.10.33/dhd.ko firmware_path=/etc/wifi/fw_bcm43438a1.bin nvram_path=/etc/wifi/nvram.txt
#sleep 1
#echo "wifi up"
#ifconfig wlan0 up 2> /tmp/wifi_error.log
#sleep 1
#echo "rmmod wifi driver"
#rmmod dhd
#sleep 1
#done
echo "wifi test end"
