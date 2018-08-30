#KERNEL_DIR := /home/work/sys/v3_A83T_qt5.3.2/linux-3.4
KERNEL_DIR := /home/root/CQA83TLinux_Qt5.8.0_bv3/linux-3.4
CROSS := arm-buildroot-linux-gnueabihf-
obj-m += swim_driver_v3_st8.o

modules:
	$(MAKE) -C $(KERNEL_DIR) M=$(shell pwd) ARCH=arm CROSS_COMPILE=$(CROSS)  -Wall
	$(CROSS)gcc swim_user.c -o stm8boot
	
clean:
	$(MAKE) -C $(KERNEL_DIR) M=$(shell pwd) clean 
	rm -rf *.o stm8boot

