.PHONY: run clean

CC=x86_64-elf-gcc
CFLAGS=-ffreestanding -Wall -Wextra -Wno-int-conversion \
		-gdwarf                         \
		-isystem src                    \
		-I src/kernel					\
		-fno-pic                        \
		-mcmodel=kernel                 \
		-mno-sse                        \
		-mno-sse2                       \
		-mno-mmx                        \
		-mno-80387                      \
		-mno-red-zone                   \
		-fno-stack-protector            \
		-fno-omit-frame-pointer         \

SOURCES = $(shell find src/ -type f -name '*.c')
HEADERS = $(shell find src/ -type f -name '*.h')

ASMFILE = $(shell find src/ -type f -name '*.asm')
OBJECTS = ${SOURCES:.c=.o} ${ASMFILE:.asm=.o}

celesteos.iso: kernel.elf
	@mkdir -p build/iso/boot/grub
	@cp kernel.elf build/iso/boot/kernel.bin
	@cp grub.cfg build/iso/boot/grub
	@grub-mkrescue -o build/celesteos.iso build/iso
	#rm -r build/iso

kernel.elf: ${OBJECTS}
	x86_64-elf-ld -n -T linker.ld -z max-page-size=0x1000 -nostdlib -o $@ ${OBJECTS}

%.o: %.c ${HEADERS}
	${CC} ${CFLAGS} -c $< -nostdlib -o $@

%.o: %.asm
	nasm -i src/asm/ -felf64 -F dwarf -g $< -o $@

run: celesteos.iso
	@qemu-system-x86_64 -smp cpus=4 -cdrom build/celesteos.iso -m 1G -no-reboot -monitor stdio -d int -D qemu.log -no-shutdown -vga vmware -s

clean:
	-rm kernel.elf
	-rm -rf build
	-rm ${OBJECTS}
	-rm qemu.log
	-rm mem
	-rm dump