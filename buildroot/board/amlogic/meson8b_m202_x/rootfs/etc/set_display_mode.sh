#!/bin/sh
HPD_STATE=/sys/class/amhdmitx/amhdmitx0/hpd_state
DISP_CAP=/sys/class/amhdmitx/amhdmitx0/disp_cap
DISP_MODE=/sys/class/display/mode

hdmi=`cat $HPD_STATE`
if [ $hdmi -eq 1 ]; then
    mode=`awk -f /etc/display/get_hdmi_mode.awk $DISP_CAP`
    echo $mode > $DISP_MODE
fi

#outputmode=$mode
outputmode=720p

case $mode in
    720*)
        fbset -fb /dev/fb0 -g 1280 720 1280 1440 32
        ;;
    1080*)
        fbset -fb /dev/fb0 -g 1920 1080 1920 2160 32
        ;;
    *)
        fbset -fb /dev/fb0 -g 1280 720 1280 1440 32
        outputmode=720p
        ;;
esac
fbset -fb /dev/fb1 -g 32 32 32 32 32

#old_state=1
#outputmode=$(cat /sys/class/display/mode)
#hpdstate=$(cat /sys/class/amhdmitx/amhdmitx0/hpd_state)
#old_state=$hpdstate
#
#outputmode=720p
#if [ "$hpdstate" = "1" ]; then
#	if [ "$outputmode" = "480cvbs" -o "$outputmode" = "576cvbs" ] ; then
#    outputmode=720p
#  fi
#else
#	if [ "$outputmode" != "480cvbs" -a "$outputmode" != "576cvbs" ] ; then
#    outputmode=576cvbs
#  fi
#fi

echo $outputmode > /sys/class/display/mode

echo 0 > /sys/class/ppmgr/ppscaler
echo 0 > /sys/class/graphics/fb0/free_scale
echo 1 > /sys/class/graphics/fb0/freescale_mode


	case $outputmode in 

		480*)
		echo 0 0 1279 719 > /sys/class/graphics/fb0/free_scale_axis
		echo 0 0 1279 719 > /sys/class/graphics/fb0/window_axis 
		;;
    
		576*)
		echo 0 0 1279 719 > /sys/class/graphics/fb0/free_scale_axis
		echo 0 0 1279 719 > /sys/class/graphics/fb0/window_axis 
		;; 

		720*)
		echo 0 0 1279 719 > /sys/class/graphics/fb0/free_scale_axis
		echo 0 0 1279 719 > /sys/class/graphics/fb0/window_axis 
		;; 

		1080*)
		echo 0 0 1919 1079 > /sys/class/graphics/fb0/free_scale_axis
		echo 0 0 1919 1079 > /sys/class/graphics/fb0/window_axis 
		;; 

		4k2k*)
		echo 0 0 1919 1079 > /sys/class/graphics/fb0/free_scale_axis
		echo 0 0 1919 1079 > /sys/class/graphics/fb0/window_axis 
		;;
 
		*)
		#outputmode= 720p
		echo 720p > /sys/class/display/mode  
		echo 0 0 1279 719 > /sys/class/graphics/fb0/free_scale_axis
		echo 0 0 1279 719 > /sys/class/graphics/fb0/window_axis 

esac

echo 0x10001 > /sys/class/graphics/fb0/free_scale
echo 0 > /sys/class/graphics/fb1/free_scale

