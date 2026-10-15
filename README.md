# CelesteOS
Hobby OS based around the Mach microkernel, aiming for binary compatibility with macOS.

## Building
CelesteOS depends on a cross compiler for `x86_64-elf` and `limine`. Both can built by running `./scripts/build_limine.sh` and
`./scripts/build_toolchain.sh`. You will need to install the packages for `nasm, qemu, xorriso`.

To build CelesteOS:
```sh
make all

# To run in QEMU (requires QEMU in $PATH)
make qemu
```

The linker entry point is `_start` in `src/kernel/asm/entry.asm`, which
prepares the bootloader-provided stack and calls `kmain`. Keep this assembly
source in the kernel target when reorganizing sources.

## Exception handling
Unhandled CPU exceptions print the saved register frame to the QEMU debug port
and halt with interrupts disabled. Page-fault diagnostics include CR2 (the
faulting virtual address) and the CPU's error code. The handler does not resume
the faulting instruction until a recovery mechanism is implemented.

The register layout in `src/kernel/cpu/isr.h` must match the stack built by
`src/kernel/asm/int.asm`. Interrupts are enabled only after the ISR/IRQ gates
are installed and both PICs have been remapped.

## Checklist
- [x] Framebuffer driver (provided by Limine)
- [x] Physical and virtual memory manager
- [x] Core IDT and GDT functions
- [x] Remapping the PIC and setting up Interrupt Service Routines
