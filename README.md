<img src="./resources/readme_logo.png">
<hr>

An open source operating system aiming for binary compatibility with Mac OS X 10.6 Snow Leopard.

## What Mac OSX version is supported?
celesteOS aims for binary compatibility with Mac OS X 10.6 Snow Leopard because modern
macOS is a moving target with frequent changes, and complex security mechanisms.

## Building
CelesteOS depends on a cross compiler for `x86_64-elf` and `limine`. Both can built by running `./scripts/build_limine.sh` and
`./scripts/build_toolchain.sh`. You will need to install the packages for `nasm, qemu, xorriso`.

To build CelesteOS:
```sh
make all

# To run in QEMU (requires QEMU in $PATH)
make qemu
```

## Checklist
- [x] Framebuffer driver (provided by Limine)
- [x] Physical and virtual memory manager
- [x] Core IDT and GDT functions
- [x] Remapping the PIC and setting up Interrupt Service Routines
