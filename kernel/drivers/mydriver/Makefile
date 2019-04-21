#
# Makefile for AC83xx BackCar Driver
# 
KERNELDIR = /home/cuishang/as/work/android-4.0.3_r1/kernel

PWD := $(shell pwd)

obj-m = buttons.o

default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
	armv7a-mediatek451_001_vfp-linux-gnueabi-strip -S buttons.ko



clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean
