# Toolchain & Executables
CC      := gcc
NASM    := nasm
LD      := ld
XORRISO := xorriso
QEMU    := qemu-system-x86_64
BASH    := bash

# Directories
BUILD_DIR := build
ISO_DIR   := $(BUILD_DIR)/iso_root
LIMINE_DIR ?= $(BUILD_DIR)/limine

# Output files
TARGET_ELF := $(BUILD_DIR)/celesteos.elf
TARGET_ISO := $(BUILD_DIR)/celesteos.iso

# Include directories
INCLUDES := -I. -Iinclude -Ikernel -Ilib -Ikernel/asm

# Compiler & assembler flags
CFLAGS := -std=c17 \
	-O2 \
	-g \
	-ffreestanding \
	-fno-stack-protector \
	-fno-pic \
	-fno-pie \
	-fno-asynchronous-unwind-tables \
	-fno-unwind-tables \
	-m64 \
	-mcmodel=kernel \
	-mno-red-zone \
	-mno-mmx \
	-mno-sse2 \
	-Wall \
	-Wextra \
	-Werror \
	-MMD -MP \
	$(INCLUDES)

NASMFLAGS ?= -f elf64 -g -F dwarf -Ikernel/asm/
LDFLAGS := -T linker.ld -nostdlib -static -z max-page-size=0x1000
LIBGCC := $(shell $(CC) -print-libgcc-file-name)

C_SRCS := kernel/kmain.c \
	kernel/gdt.c \
	kernel/tss.c \
	kernel/idt.c \
	kernel/pic.c \
	kernel/fb.c \
	kernel/pmm.c \
	kernel/pmap.c \
	kernel/ipc/kobject.c \
	kernel/ipc/scheduler.c \
	kernel/ipc/task.c \
	kernel/ipc/thread.c \
	lib/klog.c \
	lib/string.c

ASM_SRCS := kernel/asm/entry.asm \
	kernel/asm/ctx_switch.asm \
	kernel/asm/interrupts.asm \
	kernel/asm/load_gdt.asm \
	kernel/asm/tss_switch.asm \
	kernel/asm/spinlock.asm

OBJS := $(addprefix $(BUILD_DIR)/,$(C_SRCS:.c=.o)) \
	$(addprefix $(BUILD_DIR)/,$(ASM_SRCS:.asm=.o))
DEPS := $(addprefix $(BUILD_DIR)/,$(C_SRCS:.c=.d))

# Targets
.PHONY: all kernel iso limine qemu qemu-debug run clean test-tss

all: iso

# Kernel build target
kernel: $(TARGET_ELF)

# Tests
test-tss:
	@mkdir -p $(BUILD_DIR)/tests
	$(CC) -std=c17 -Wall -Wextra -Werror -I. -Iinclude \
		tests/tss_test.c kernel/tss.c kernel/gdt.c -o $(BUILD_DIR)/tests/tss_test
	$(BUILD_DIR)/tests/tss_test

# Limine bootloader
limine:
	LIMINE_DIR="$(LIMINE_DIR)" $(BASH) ./build_limine.sh

# ISO image: depends on the kernel ELF and Limine bootloader
iso: $(TARGET_ISO)
$(TARGET_ISO): $(TARGET_ELF) limine.conf | limine
	@command -v $(XORRISO) >/dev/null || { echo "Error: $(XORRISO) is required to build the bootable ISO." >&2; exit 1; }
	mkdir -p $(ISO_DIR)/boot/limine $(ISO_DIR)/EFI/BOOT
	cp $(TARGET_ELF) $(ISO_DIR)/boot/kernel.elf
	cp limine.conf $(ISO_DIR)/boot/limine/limine.conf
	cp $(LIMINE_DIR)/limine-bios.sys $(LIMINE_DIR)/limine-bios-cd.bin \
		$(LIMINE_DIR)/limine-uefi-cd.bin $(ISO_DIR)/boot/limine/
	cp $(LIMINE_DIR)/BOOTX64.EFI $(ISO_DIR)/EFI/BOOT/BOOTX64.EFI
	$(XORRISO) -as mkisofs -R -r -J \
		-b boot/limine/limine-bios-cd.bin -no-emul-boot -boot-load-size 4 -boot-info-table \
		--efi-boot boot/limine/limine-uefi-cd.bin -efi-boot-part --efi-boot-image \
		--protective-msdos-label $(ISO_DIR) -o $(TARGET_ISO)
	$(LIMINE_DIR)/limine bios-install $(TARGET_ISO)

# QEMU run target
qemu: iso
	$(QEMU) -cdrom $(TARGET_ISO) \
		-m 512M \
		-gdb tcp:127.0.0.1:1234 \
		-debugcon stdio \
		-no-reboot -no-shutdown \
		-d int,cpu_reset \
		-D qemu.log

run: qemu

# Start QEMU paused, waiting for a debugger on tcp:1234
qemu-debug: iso
	$(QEMU) -cdrom $(TARGET_ISO) \
		-m 512M \
		-gdb tcp:127.0.0.1:1234 \
		-S \
		-pidfile $(BUILD_DIR)/qemu.pid \
		-debugcon stdio \
		-no-reboot -no-shutdown \
		-d int,cpu_reset \
		-D qemu.log

# Compile C sources
$(BUILD_DIR)/%.o: %.c Makefile
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Assemble NASM sources
$(BUILD_DIR)/%.o: %.s Makefile
	@mkdir -p $(dir $@)
	$(NASM) $(NASMFLAGS) $< -o $@

$(BUILD_DIR)/%.o: %.asm Makefile
	@mkdir -p $(dir $@)
	$(NASM) $(NASMFLAGS) $< -o $@

$(BUILD_DIR)/%.o: %.nasm Makefile
	@mkdir -p $(dir $@)
	$(NASM) $(NASMFLAGS) $< -o $@

# Link kernel ELF
$(TARGET_ELF): $(OBJS) linker.ld
	@mkdir -p $(BUILD_DIR)
	$(LD) $(LDFLAGS) $(OBJS) $(LIBGCC) -o $@

clean:
	rm -rf $(BUILD_DIR)

-include $(DEPS)
