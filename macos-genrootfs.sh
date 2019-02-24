#!/bin/sh

PRODUCT=mini210

echo "Generating rootfs for Android..."
sudo rm -fr rootfs_dir
sudo cp -a out/target/product/${PRODUCT}/root rootfs_dir 
sudo cp -a out/target/product/${PRODUCT}/system rootfs_dir/
sudo cp -a out/target/product/${PRODUCT}/data rootfs_dir/
sudo chmod 755 rootfs_dir/system/etc/dhcpcd/dhcpcd-run-hooks 
sudo chown -R 1014:2000 rootfs_dir/system/etc/dhcpcd/dhcpcd-hooks
sudo chown -R 1000:1000 rootfs_dir/data
echo "...done"

if [ "${PRODUCT}" = "mini210" ]; then
	echo "Create device file..."
	sudo mkdir rootfs_dir/dev/input
	sudo mknod rootfs_dir/dev/tty c 5 0
	sudo mknod rootfs_dir/dev/console c 5 1
	sudo mknod rootfs_dir/dev/fb0 c 29 0
	sudo mknod rootfs_dir/dev/pointercal c 10 119
	sudo mknod rootfs_dir/dev/input/event1  c 13 65
	sudo mknod rootfs_dir/dev/ts-if c 10 185
	sudo mknod rootfs_dir/dev/touchscreen c 10 180
	sudo mknod rootfs_dir/dev/touchscreen-1wire c 10 181
	sudo mknod rootfs_dir/dev/s3c2410_serial0 c 204 64
	sudo mknod rootfs_dir/dev/s3c2410_serial3 c 204 67
	echo "...done"

	echo "Install prebuilt packages..."
	if [ -f vendor/friendly-arm/common/busybox-bin.tgz ]; then
		sudo tar xf vendor/friendly-arm/common/busybox-bin.tgz -C rootfs_dir/system
		sudo ln -sf /system/busybox/bin/busybox rootfs_dir/system/bin/sh
		sudo mkdir -p rootfs_dir/bin && sudo ln -sf /system/busybox/bin/sh rootfs_dir/bin/sh
	fi
	if [ -f vendor/friendly-arm/common/iwtools-bin.tgz ]; then
		sudo tar xf vendor/friendly-arm/common/iwtools-bin.tgz -C rootfs_dir/system/bin
	fi
	echo "...done"

	echo "Install kernel modules and firmware..."
	if [ -f vendor/friendly-arm/mini210/firmware.tgz ]; then
		sudo tar xf vendor/friendly-arm/mini210/firmware.tgz -C rootfs_dir/system/etc
	fi
	if [ -f vendor/friendly-arm/mini210/kernel-modules.tgz ]; then
		sudo tar xf vendor/friendly-arm/mini210/kernel-modules.tgz -C rootfs_dir/system/lib
		find rootfs_dir/system/lib/modules/ -name modules.* \
				-o -name source -o -name build | xargs sudo rm -rf
	fi
	echo "...done"

	echo "Install proprietary-open files..."
	[ -f FriendlyARMData.tgz ] && tar xf FriendlyARMData.tgz -C rootfs_dir
	echo "...done"
	echo "Install Root files..."
	[ -f "vendor/friendly-arm/mini210/ics4_root_patch.tgz" ] && \
        	sudo tar xzf "vendor/friendly-arm/mini210/ics4_root_patch.tgz" -C rootfs_dir
	echo "...done"

	echo "Install google apps ..."
	[ -f "vendor/friendly-arm/mini210/gapps-ics-20120429.tgz" ] && \
        	sudo tar xzf "vendor/friendly-arm/mini210/gapps-ics-20120429.tgz" -C rootfs_dir
	echo "...done"

	sudo cp -a vendor/friendly-arm/mini210/rootdir/* rootfs_dir/
	# cp -a vendor/friendly-arm/mini210/3rd-apps/* rootfs_dir/
	
	if [ -d rootfs_dir/data/app ]; then
		sudo chown -R 1000:1000 rootfs_dir/data/app
		sudo chmod 775 rootfs_dir/data/app

        sudo chown -R 1000:1000 rootfs_dir/data/system
        sudo chmod 775 rootfs_dir/data/system

        sudo chmod 775 rootfs_dir/data/dalvik-cache
	fi

	if [ -d rootfs_dir/system/vendor ]; then
		find rootfs_dir/system/vendor -name *.so | xargs sudo chmod 755
	fi

	find rootfs_dir/ -name CVS -type d  | xargs sudo rm -rf

	if [ "$1" == "image" ]; then
        	mkyaffs2image-128M rootfs_dir rootfs_android_mac.img   
        	mkyaffs2image-mlc2 rootfs_dir rootfs_android_mac-mlc2.img
        	md5 rootfs_android_mac.img
        	md5 rootfs_android_mac-mlc2.img
	fi
fi

