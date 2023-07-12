ARCH ?= riscv
CROSS_COMPILE ?= riscv64-linux-gnu-
Q := @
ifeq ($(V), 1)
	Q :=
endif

.PHONY: prepare-third-party
prepare-third-party:
	$(Q)ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) scripts/prepare-third-party.sh