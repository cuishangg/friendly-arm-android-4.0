#!/system/bin/sh

# disable boot animation for a faster boot sequence when needed
boot_anim=`getprop ro.kernel.android.bootanim`
case "$boot_anim" in
    0)  setprop debug.sf.nobootanimation 1
    ;;
esac

# FA customize
#logcat *:V > /data/log.txt &

MODULE_PATH=/system/lib/modules/`uname -r`
if [ ! -f ${MODULE_PATH}/modules.dep.bb ]; then
	depmod
fi

# Audio, video, input
modprobe snd-soc-wm8960
modprobe snd-soc-mini210-wm8960
modprobe ov9650
modprobe tvp5150_tiny210
modprobe goodix_touch

fa_codec_ctrl mini210

# Wi-Fi support
modprobe libertas_sdio
modprobe ath9k_htc
modprobe rt73usb
modprobe rt2800usb
modprobe rtl8192cu
modprobe zd1211rw
setprop wlan.driver.status builtin

# Ehternet mac address

change_mac_address()
{
    echo "set macaddress" >> /data/eth0-log.txt
    /system/busybox/sbin/ifconfig eth0 down
    /system/busybox/sbin/ifconfig eth0 hw ether $1
    /system/busybox/sbin/ifconfig eth0 up
}

if [ -f /data/system/eth0-macaddress.conf ] ; then
	echo "found /data/system/eth0-macaddress.conf" >> /data/eth0-log.txt
	source  /data/system/eth0-macaddress.conf
	DEV_ADDR=`cat /sys/class/net/eth0/address`
	if [ "$DEV_ADDR" = "00:00:00:00:00:00" ]; then
		change_mac_address $MAC
	elif [ "$DEV_ADDR" != "$MAC" ]; then
		change_mac_address $MAC
	fi
fi

