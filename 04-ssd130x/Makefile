obj-m := ssd130x.o
ssd130x-objs += main.o ssd1306.o

KDIR ?= /lib/modules/$(shell uname -r)/build

user_programs := test_disp

%:
	ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- $(MAKE) -C $(KDIR) M=$(PWD) $@

new_device:
	echo ssd1306 0x3c | sudo tee /sys/bus/i2c/devices/i2c-2/new_device

delete_device:
	echo 0x3c | sudo tee /sys/bus/i2c/devices/i2c-2/delete_device

uprogram:
	$(CC) -static user/test_disp.c -o test_disp

uclean:
	rm -rf $(user_programs)