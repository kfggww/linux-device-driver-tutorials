ifeq ($(KERNELRELEASE), )
	P ?= arm
	include ../scripts/Makefile.envs
	include ../scripts/Makefile.rules
endif

obj-m = cdev02.o

client: client.c
	$(CROSS_COMPILE)gcc -static $< -o $@