ifeq ($(KERNELRELEASE), )
	P ?= arm
	include ../scripts/Makefile.envs
	include ../scripts/Makefile.rules
endif

obj-m = cdev02.o
CFLAGS_cdev02.o += -g

client: client.c
	$(CROSS_COMPILE)gcc -static $< -o $@