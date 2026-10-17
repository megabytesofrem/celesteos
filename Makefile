# Toolchain & Executables
CC      := gcc
NASM    := nasm
LD      := ld
XORRISO := xorriso
QEMU    := qemu-system-x86_64
BASH    := bash

# Directories
SRC_DIR   := src
BUILD_DIR := build
ISO_DIR   := $(BUILD_DIR)/iso_root
LIMINE_DIR:= $(BUILD_DIR)/limine

# Output Files
TARGET_ELF := $(BUILD_DIR)/celesteos.elf
TARGET_ISO := $(BUILD_DIR)/celesteos.iso

# Include Directories
INCLUDES := -Isrc \
            -Isrc/stdlib \
            -Isrc/kernel \
            -Isrc/kernel/memory \
            -Isrc/kernel/drivers \
            -Isrc/kernel/asm

# Compiler & Assembler Flags
CFLAGS := -std=c17 \
          -ffreestanding \
          -mno-red-zone \
          -mcmodel=kernel \
          -mno-mmx \
          -mno-sse2 \
          -fno-stack-protector \
          -Wall \
          -Wno-unused-variable \
          -Wextra \
          $(INCLUDES)

NASMFLAGS := -f elf64 -g -F dwarf

LDFLAGS := -T linker.ld -nostdlib -static

# Source Files
C_SRCS := src/kernel/kmain.c \
          src/kernel/cpuid.c \
          src/kernel/gdt.c \
          src/kernel/idt.c \
          src/kernel/isr.c \
          src/kernel/pic.c \
          src/kernel/memory/pmm.c \
          src/kernel/memory/pmap.c \
          src/kernel/memory/vm_map.c \
          src/kernel/memory/kmalloc.c \
          src/stdlib/string.c \
          src/stdlib/libkernel.c \
          src/kernel/drivers/framebuffer.c \
          src/kernel/mach/loader.c \
          src/kernel/initrd.c \
          src/kernel/task.c
          

ASM_SRCS := src/kernel/asm/entry.asm \
            src/kernel/asm/load_gdt.asm \
            src/kernel/asm/int.asm \
            src/kernel/asm/tss_ring3.asm

# Map sources to object files in build directory
OBJS := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(C_SRCS)) \
        $(patsubst $(SRC_DIR)/%.asm,$(BUILD_DIR)/%.o,$(ASM_SRCS))

# Libgcc path lookup for freestanding 64-bit divide/stubs
LIBGCC := $(shell $(CC) -print-libgcc-file-name)

# Targets
.PHONY: all iso limine_bootloader qemu clean

all: iso

# Compile C sources
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Assemble NASM sources
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.asm
	@mkdir -p $(dir $@)
	$(NASM) $(NASMFLAGS) $< -o $@

# Link Executable
$(TARGET_ELF): $(OBJS)
	@mkdir -p $(BUILD_DIR)
	$(LD) $(LDFLAGS) $(OBJS) $(LIBGCC) -o $@

# Limine Bootloader target
limine:
	@mkdir -p $(LIMINE_DIR)
	LIMINE_DIR="$(LIMINE_DIR)" $(BASH) scripts/build_limine.sh

# Build ISO target
iso: $(TARGET_ELF) limine
	@mkdir -p $(ISO_DIR)/boot
	cp $(TARGET_ELF) $(ISO_DIR)/boot/kernel.elf
	cp $(LIMINE_DIR)/limine-bios.sys $(ISO_DIR)/
	cp $(LIMINE_DIR)/limine-bios-cd.bin $(ISO_DIR)/
	cp limine.conf $(ISO_DIR)/
	$(XORRISO) -as mkisofs -b limine-bios-cd.bin \
		-no-emul-boot -boot-load-size 4 -boot-info-table \
		$(ISO_DIR) -o $(TARGET_ISO)
	$(LIMINE_DIR)/limine bios-install $(TARGET_ISO)

# QEMU target
qemu: iso
	$(QEMU) -cdrom $(TARGET_ISO) \
		-m 512M \
		-gdb tcp:127.0.0.1:1234 \
		-debugcon stdio \
		-no-reboot -no-shutdown \
		-d int,cpu_reset \
		-D qemu.log

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR)