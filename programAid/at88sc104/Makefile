#
# Makefile for the kernel tpm device drivers.
#
#obj-$(CONFIG_ST88SC104) += at88sc0104.o
#obj-m += st88sc104.o
#KERNEL_DIR := /home/work/sys/v3_A83T_qt5.3.2/linux-3.4
KERNEL_DIR := /home/root/CQA83TLinux_Qt5.8.0_bv3/linux-3.4
CROSS := arm-buildroot-linux-gnueabihf-
obj-m += at88sc0104.o

modules:
	$(MAKE) -C $(KERNEL_DIR) M=$(shell pwd) ARCH=arm CROSS_COMPILE=$(CROSS)
	$(CROSS)gcc at88sc104_test_app.c -o optAt88
	
clean:
	$(MAKE) -C $(KERNEL_DIR) M=$(shell pwd) clean 
	rm -rf *.o a.out optAt88
