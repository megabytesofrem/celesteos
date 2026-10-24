CC ?= gcc

BUILD_DIR := build
KERNEL := $(BUILD_DIR)/kernel.elf
ISO_ROOT := $(BUILD_DIR)/iso_root
ISO := $(BUILD_DIR)/celesteos.iso
LIMINE_DIR ?= $(BUILD_DIR)/limine
QEMU ?= qemu-system-x86_64
QEMUFLAGS ?= -m 2G
NASM ?= nasm
NASMFLAGS ?= -f elf64
C_SOURCES := $(shell find kernel lib servers -type f -name '*.c' -print)
ASM_SOURCES := $(shell find kernel lib servers -type f \( -name '*.s' -o -name '*.asm' -o -name '*.nasm' \) -print)
C_OBJECTS := $(patsubst %.c,$(BUILD_DIR)/%.o,$(C_SOURCES))
ASM_OBJECTS := $(patsubst %.s,$(BUILD_DIR)/%.o,$(filter %.s,$(ASM_SOURCES))) \
	$(patsubst %.asm,$(BUILD_DIR)/%.o,$(filter %.asm,$(ASM_SOURCES))) \
	$(patsubst %.nasm,$(BUILD_DIR)/%.o,$(filter %.nasm,$(ASM_SOURCES)))
OBJECTS := $(C_OBJECTS) $(ASM_OBJECTS)
DEPFILES := $(C_OBJECTS:.o=.d)

CPPFLAGS := -I. -Iinclude
CFLAGS := -std=gnu11 -O2 -Wall -Wextra -Werror \
	-ffreestanding -fno-stack-protector -fno-pic -fno-pie \
	-fno-asynchronous-unwind-tables -fno-unwind-tables \
	-m64 -mcmodel=kernel -mno-red-zone
LDFLAGS := -nostdlib -no-pie -Wl,-T,linker.ld -Wl,-z,max-page-size=0x1000

.PHONY: all kernel limine run clean

all: $(ISO)

kernel: $(KERNEL)

limine:
	LIMINE_DIR="$(LIMINE_DIR)" bash ./build_limine.sh

$(ISO): $(KERNEL) limine.conf | limine
	@command -v xorriso >/dev/null || { echo "Error: xorriso is required to build the bootable ISO." >&2; exit 1; }
	mkdir -p $(ISO_ROOT)/boot/limine $(ISO_ROOT)/EFI/BOOT
	cp $(KERNEL) $(ISO_ROOT)/boot/kernel.elf
	cp limine.conf $(ISO_ROOT)/boot/limine/limine.conf
	cp $(LIMINE_DIR)/limine-bios.sys $(LIMINE_DIR)/limine-bios-cd.bin \
		$(LIMINE_DIR)/limine-uefi-cd.bin $(ISO_ROOT)/boot/limine/
	cp $(LIMINE_DIR)/BOOTX64.EFI $(ISO_ROOT)/EFI/BOOT/BOOTX64.EFI
	xorriso -as mkisofs -R -r -J \
		-b boot/limine/limine-bios-cd.bin -no-emul-boot -boot-load-size 4 -boot-info-table \
		--efi-boot boot/limine/limine-uefi-cd.bin -efi-boot-part --efi-boot-image \
		--protective-msdos-label $(ISO_ROOT) -o $(ISO)
	$(LIMINE_DIR)/limine bios-install $(ISO)

run: $(ISO)
	$(QEMU) $(QEMUFLAGS) -cdrom $(ISO) -boot d

$(KERNEL): $(OBJECTS) linker.ld
	@mkdir -p $(@D)
	$(CC) $(LDFLAGS) -o $@ $(OBJECTS)

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@

$(BUILD_DIR)/%.o: %.s
	@mkdir -p $(@D)
	$(NASM) $(NASMFLAGS) $< -o $@

$(BUILD_DIR)/%.o: %.asm
	@mkdir -p $(@D)
	$(NASM) $(NASMFLAGS) $< -o $@

$(BUILD_DIR)/%.o: %.nasm
	@mkdir -p $(@D)
	$(NASM) $(NASMFLAGS) $< -o $@

clean:
	rm -rf $(BUILD_DIR)

-include $(DEPFILES)
