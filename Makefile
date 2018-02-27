KERNEL_DIR := /home/work/sys/v3_A83T_qt5.3.2/linux-3.4

obj-m += v3_stm8_swim_driver.o

modules:
	$(MAKE) -C $(KERNEL_DIR) M=$(shell pwd) ARCH=arm CROSS_COMPILE=arm-none-linux-gnueabi-
	arm-none-linux-gnueabi-gcc swim_user.c -o app_swim
clean:
	$(MAKE) -C $(KERNEL_DIR) M=$(shell pwd) clean 
	rm -rf *.o swim_app

