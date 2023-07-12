ARCH ?= riscv
CROSS_COMPILE ?= riscv64-linux-gnu-

QEMU = qemu-system-riscv64
QEMU_OPTIONS += -machine virt \
				-smp 4 \
				-kernel $(shell find third-party -maxdepth 1 -name linux*)/build/arch/$(ARCH)/boot/Image \
				-drive file=$(shell pwd)/third-party/rootfs.ext4,format=raw,id=hd0 \
				-device virtio-blk-device,drive=hd0 \
				-fsdev local,id=lddt,path=$(shell pwd),security_model=passthrough \
				-device virtio-9p-pci,fsdev=lddt,mount_tag=lddtsrc \
				-append "root=/dev/vda ro console=ttyS0 init=/bin/init.sh" \
				-nographic

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
