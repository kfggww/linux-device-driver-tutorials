ARCH ?= riscv
CROSS_COMPILE ?= riscv64-linux-gnu-

obj-m = testtimer.o

KDIR ?= $(shell find ../third-party -maxdepth 1 -name linux*)/build

all:
	ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) $(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) $(MAKE) -C $(KDIR) M=$(PWD) clean

.PHONY: all clean