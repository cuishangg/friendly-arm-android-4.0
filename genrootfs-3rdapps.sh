#!/bin/sh

PRODUCT=mini210

echo -n "Generating rootfs for Android..."
rm -fr rootfs_dir
cp -a out/target/product/${PRODUCT}/root/ rootfs_dir
cp -a out/target/product/${PRODUCT}/system/ rootfs_dir/
cp -a out/target/product/${PRODUCT}/data/ rootfs_dir/
chown root:root rootfs_dir -R
chmod 755 rootfs_dir/system/etc/dhcpcd/dhcpcd-run-hooks
chown 1014:2000 rootfs_dir/system/etc/dhcpcd/dhcpcd-hooks -R
chown 1000:1000 rootfs_dir/data -R
echo "...done"

function mvapk()
{
	mv $1 rootfs_dir/mnt/apps
	echo "    $1"
}

if [ "${PRODUCT}" = "mini210" ]; then
	echo -n "Create device file..."
	mkdir rootfs_dir/dev/input
	mknod rootfs_dir/dev/tty c 5 0
	mknod rootfs_dir/dev/console c 5 1
	mknod rootfs_dir/dev/fb0 c 29 0
	mknod rootfs_dir/dev/pointercal c 10 119
	mknod rootfs_dir/dev/input/event1  c 13 65
	mknod rootfs_dir/dev/ts-if c 10 185
	mknod rootfs_dir/dev/touchscreen c 10 180
	mknod rootfs_dir/dev/touchscreen-1wire c 10 181
	mknod rootfs_dir/dev/s3c2410_serial0 c 204 64
	mknod rootfs_dir/dev/s3c2410_serial3 c 204 67
	echo "...done"

	echo -n "Install prebuilt packages..."
	if [ -f vendor/friendly-arm/common/busybox-bin.tgz ]; then
		tar xf vendor/friendly-arm/common/busybox-bin.tgz -C rootfs_dir/system
		ln -sf /system/busybox/bin/busybox rootfs_dir/system/bin/sh
		mkdir -p rootfs_dir/bin && ln -sf /system/busybox/bin/sh rootfs_dir/bin/sh
	fi
	if [ -f vendor/friendly-arm/common/iwtools-bin.tgz ]; then
		tar xf vendor/friendly-arm/common/iwtools-bin.tgz -C rootfs_dir/system/bin
	fi
	echo "...done"

	echo -n "Install kernel modules and firmware..."
	if [ -f vendor/friendly-arm/mini210/firmware.tgz ]; then
		tar xf vendor/friendly-arm/mini210/firmware.tgz -C rootfs_dir/system/etc
	fi
	if [ -f vendor/friendly-arm/mini210/kernel-modules.tgz ]; then
		tar xf vendor/friendly-arm/mini210/kernel-modules.tgz -C rootfs_dir/system/lib
		find rootfs_dir/system/lib/modules/ -name modules.* \
				-o -name source -o -name build | xargs rm -rf
	fi
	echo "...done"

	echo -n "Install proprietary-open files..."
	[ -f FriendlyARMData.tgz ] && tar xf FriendlyARMData.tgz -C rootfs_dir
	echo "...done"

    echo -n "Install Root files..."
    [ -f "vendor/friendly-arm/mini210/ics4_root_patch.tgz" ] && \
        tar xzf "vendor/friendly-arm/mini210/ics4_root_patch.tgz" -C rootfs_dir
    echo "...done"

	cp vendor/friendly-arm/mini210/rootdir/* rootfs_dir/ -af
    cp vendor/friendly-arm/mini210/3rd-apps/* rootfs_dir/ -af
	
	if [ -d rootfs_dir/data/app ]; then
		chown 1000:1000 rootfs_dir/data/app -R
		chmod 775 rootfs_dir/data/app

        chown 1000:1000 rootfs_dir/data/system -R
        chmod 775 rootfs_dir/data/system

        chmod 775 rootfs_dir/data/dalvik-cache
	fi

	if [ -d rootfs_dir/system/vendor ]; then
		find rootfs_dir/system/vendor -name *.so | xargs chmod 755
	fi

    find rootfs_dir/ -name CVS -type d  | xargs rm -rf
    if [ "$1" == "image" ]; then
        mkyaffs2image-128M rootfs_dir rootfs_android.img
        md5sum rootfs_android.img
    fi
fi


