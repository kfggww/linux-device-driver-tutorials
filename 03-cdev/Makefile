ifeq ($(KERNELRELEASE), )
	P ?= arm
	include ../scripts/Makefile.envs
	include ../scripts/Makefile.rules
endif

obj-m = cdev03.o
CFLAGS_cdev03.o += -g

poll_reader: poll_reader.c
	$(CROSS_COMPILE)gcc -static $< -o $@