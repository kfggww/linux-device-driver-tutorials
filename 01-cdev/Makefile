ifeq ($(KERNELRELEASE), )
	P ?= arm
	include ../scripts/Makefile.envs
	include ../scripts/Makefile.rules
endif

obj-m = cdev01.o
CFLAGS_cdev01.o += -g
