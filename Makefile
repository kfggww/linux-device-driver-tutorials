P ?= arm
include scripts/Makefile.envs

# options that qemu use when start the guest machine
QEMU_OPTIONS = -machine $(machine_type) \
			   -smp 4 \
			   -kernel $(KDIR)/arch/$(ARCH)/boot/zImage \
			   -drive file=$(PROJECT_DIR)/third-party/rootfs.ext4,format=raw,id=hda \
			   -device virtio-blk-device,drive=hda \
			   -fsdev local,id=lddt,path=$(PROJECT_DIR),security_model=passthrough \
			   -device virtio-9p-device,fsdev=lddt,mount_tag=lddt \
			   -append "root=/dev/vda ro console=$(console_name) init=/bin/init.sh" \
			   -nographic

ifeq ($(ARCH), arm)
QEMU = qemu-system-arm
machine_type = vexpress-a9
dtb_file = $(KDIR)/arch/$(ARCH)/boot/dts/vexpress-v2p-ca9.dtb
console_name = ttyAMA0
QEMU_OPTIONS += -dtb $(dtb_file)
else ifeq ($(ARCH), riscv)
QEMU = qemu-system-riscv64
machine_type = virt
console_name = ttyS0
else
$(error Only support "arm" or "riscv")
endif

# Set V=1 to print out the commands run by make
Q := @
ifeq ($(V), 1)
Q :=
endif

# Set parallel make for "kernel_all" and "bbox_all" target
nproc = $(if $(filter %all, $@), -j$$(nproc), )

# Commands for prepare-third-party
define prepare_third_party
ARCH=$(ARCH); \
CROSS_COMPILE=$(CROSS_COMPILE); \
KERNEL_VERSION=$(KERNEL_VERSION); \
BUSYBOX_VERSION=$(BUSYBOX_VERSION); \
source scripts/prepare-third-party.sh; \
$1
endef

prepare-third-party:
	$(Q)$(call prepare_third_party, prepare_all)

rootfs:
	$(Q)$(call prepare_third_party, build_rootfs)

qemu_debug:
	$(Q)$(QEMU) $(QEMU_OPTIONS) -s -S

qemu_run:
	$(Q)$(QEMU) $(QEMU_OPTIONS)

kernel_%:
	$(Q)ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) $(MAKE) -C $(KDIR) $(@:kernel_%=%) $(nproc)

bbox_%:
	$(Q)ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) $(MAKE) -C $(BBOXDIR) $(@:bbox_%=%) $(nproc)

.PHONY: prepare-third-party qemu_debug qemu_run
