obj-m := ssd130x.o
ssd130x-objs += main.o ssd1306.o

KDIR ?= /lib/modules/$(shell uname -r)/build
user_programs := server2 player test-oled

%:
	ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- $(MAKE) -C $(KDIR) M=$(PWD) $@

new_device:
	echo ssd1306 0x3c | sudo tee /sys/bus/i2c/devices/i2c-2/new_device

delete_device:
	echo 0x3c | sudo tee /sys/bus/i2c/devices/i2c-2/delete_device

user-host: user/server2.c user/resize.cu
	nvcc $^ -o server2

user-bbb: player test-oled

player: user/player.c
	$(CROSS_COMPILE)gcc $^ -o $@

test-oled: user/test-oled.c
	$(CROSS_COMPILE)gcc $^ -o $@

user-clean:
	rm -rf $(user_programs)

.PHONY: user-host user-bbb user-clean