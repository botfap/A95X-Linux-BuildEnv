#!/bin/sh

delay=10
total=3000

fudev=/dev/sda
CNT=/lib/preinit/cnt

while true
do

if [ ! -e "$fudev" ]; then
    echo "Please insert a U disk to start test!"
    exit 0
fi

if [ -e $CNT ]
then
    cnt=`cat $CNT`

    if [ $cnt == "off" ]; then
        echo "Auto reboot is off"
	exit 0
    fi
else
    echo reset Reboot count.
    echo 0 > $CNT
fi

echo  Reboot after $delay seconds. 

let "cnt=$cnt+1"

if [ $cnt -ge $total ] 
then 
    echo AutoReboot Finisned. 
    echo "off" > $CNT
    echo "do cleaning ..."
    rm -rf /lib/preinit/
    #rm -f $CNT
    exit 0
fi

echo $cnt > $CNT
echo "current cnt = $cnt"
sleep $delay
reboot
exit 0
done
