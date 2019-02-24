#!/bin/sh

ROOTFS_DIR=rootfs_dir-s

[ -d rootfs_dir ] || {
	./genrootfs.sh
}

echo -n "Removing ${ROOTFS_DIR}..."
rm -rf ${ROOTFS_DIR}
echo "...done"

echo -n "Preparing rootfs for Serial-IF touch..."
cp rootfs_dir ${ROOTFS_DIR} -af
echo "...done"

#----------------------------------------------------------
echo -n "Patching files..."

mknod ${ROOTFS_DIR}/dev/s3c2410_serial2 c 204 66

cat > ${ROOTFS_DIR}/system/etc/ts.detected <<EOF
CHECK_1WIRE=N
EOF

cat > ${ROOTFS_DIR}/system/etc/friendlyarm-ts-input.conf <<EOF
#TSLIB_TSDEVICE=/dev/touchscreen-1wire
TSLIB_TSDEVICE=/dev/s3c2410_serial2
EOF

echo "...done"

