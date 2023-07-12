ARCH ?= riscv
CROSS_COMPILE ?= riscv64-linux-gnu-

obj-m := hello_world.o

KERNEL_DIR = $(shell realpath $$(find ../third-party -maxdepth 1 -name linux*))/build

all:
	ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) $(MAKE) -C $(KERNEL_DIR) M=$(PWD) modules

clean:
	ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) $(MAKE) -C $(KERNEL_DIR) M=$(PWD) clean

.PHONY: all clean