ARCH ?= riscv
CROSS_COMPILE ?= riscv64-linux-gnu-

# options that qemu use when start the guest machine
QEMU_OPTIONS = -machine $(machine_type) \
			   -smp 4 \
			   -kernel $(shell find third-party -maxdepth 1 -name linux*)/build-$(ARCH)/arch/$(ARCH)/boot/zImage \
			   -drive file=$(shell pwd)/third-party/rootfs.ext4,format=raw,id=hda \
			   -device virtio-blk-device,drive=hda \
			   -fsdev local,id=lddt,path=$(shell pwd),security_model=passthrough \
			   -device virtio-9p-device,fsdev=lddt,mount_tag=lddt \
			   -append "root=/dev/vda ro console=$(console_name) init=/bin/init.sh" \
			   -nographic

ifeq ($(ARCH), arm)
	QEMU = qemu-system-arm
	machine_type = vexpress-a9
	dtb_file = $(shell find third-party -maxdepth 1 -name linux*)/build-$(ARCH)/arch/$(ARCH)/boot/dts/vexpress-v2p-ca9.dtb
	console_name = ttyAMA0
	QEMU_OPTIONS += -dtb $(dtb_file)
else ifeq ($(ARCH), riscv)
	QEMU = qemu-system-riscv64
	machine_type = virt
	console_name = ttyS0
else
	err := $(error ERROR: ARCH NOT supported)
endif

# Set V=1 to print out the command run by make
Q := @
ifeq ($(V), 1)
	Q :=
endif

prepare-third-party:
	$(Q)ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) scripts/prepare-third-party.sh

qemu_debug:
	$(Q)$(QEMU) $(QEMU_OPTIONS) -s -S

qemu_run:
	$(Q)$(QEMU) $(QEMU_OPTIONS)

.PHONY: prepare-third-party qemu_debug qemu_run
