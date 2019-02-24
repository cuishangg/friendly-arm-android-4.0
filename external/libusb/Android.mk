LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	core.c \
	descriptor.c \
	io.c \
	sync.c \
	os/linux_usbfs.c

LOCAL_C_INCLUDES += \
	$(KERNEL_HEADERS) \
	external/libusb/ \
	external/libusb/os

LOCAL_CFLAGS := -DOS_LINUX

LOCAL_MODULE_TAGS := eng
LOCAL_PRELINK_MODULE := false
LOCAL_MODULE:= libusb
include $(BUILD_SHARED_LIBRARY)
