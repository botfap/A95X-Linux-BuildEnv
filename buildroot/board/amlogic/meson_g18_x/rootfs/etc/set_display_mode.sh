#!/bin/sh
HPD_STATE=/sys/class/amhdmitx/amhdmitx0/hpd_state
DISP_CAP=/sys/class/amhdmitx/amhdmitx0/disp_cap
DISP_MODE=/sys/class/display/mode

hdmi=`cat $HPD_STATE`
if [ $hdmi -eq 1 ]; then
    mode=`awk -f /etc/display/get_hdmi_mode.awk $DISP_CAP`
    echo $mode > $DISP_MODE
fi

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

#for recovery
#old_state=1
#hpdstate=$(cat /sys/class/amhdmitx/amhdmitx0/hpd_state)
#display_mode=$(cat /sys/class/display/mode)
#old_state=$hpdstate
#
#echo 1 > /sys/class/graphics/fb0/blank
#echo 1 > /sys/class/graphics/fb1/blank
#
##if [ $hpdstate -eq 1 ] ; then
##    echo 720p > /sys/class/display/mode
##    echo 0 0 1280 720 0 > /sys/class/ppmgr/ppscaler_rect
##
##elif [ $hpdstate -eq 0 ] ; then
##    echo 576cvbs > /sys/class/display/mode	
##    echo 0 0 768 576 0 > /sys/class/ppmgr/ppscaler_rect
##    
##else
##    echo 720p > /sys/class/display/mode
##    echo 0 0 1280 720 0 > /sys/class/ppmgr/ppscaler_rect
##fi
#
##default HDMI 720p
#echo 720p > /sys/class/display/mode
#echo 0 0 1280 720 0 > /sys/class/ppmgr/ppscaler_rect
#
#echo 0 0 1280 720 0 0 18 18 > /sys/class/display/axis
#echo 0 > /sys/class/graphics/fb0/freescale_mode
#echo 0 > /sys/class/graphics/fb1/freescale_mode
#echo 0 0 1279 719 > /sys/class/graphics/fb0/free_scale_axis
#echo 0x10001 > /sys/class/graphics/fb0/free_scale
#echo 0 > /sys/class/graphics/fb1/free_scale
#
#echo 0 > /sys/class/graphics/fb0/blank
#echo 0 > /sys/class/graphics/fb1/blank
