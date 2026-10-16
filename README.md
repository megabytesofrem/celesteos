<img src="./resources/readme_logo.png">
<hr>

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

## Checklist
- [x] Framebuffer driver (provided by Limine)
- [x] Physical and virtual memory manager
- [x] Core IDT and GDT functions
- [x] Remapping the PIC and setting up Interrupt Service Routines

## Kernel heap

The heap uses first-fit allocation, block splitting, and free-block coalescing.
It starts with one 4 KiB page and grows on demand using virtually contiguous
pages backed by independent physical frames. Failed growth releases newly mapped
backing pages and VM entries without changing existing heap blocks; empty
page-table structures are retained by the VMM.

The kernel VM map is bootstrapped without heap allocations. Its fixed
1,024-entry array currently tracks one entry per page, limiting the heap to at
most 4 MiB (less if other allocations use that map). Freeing a heap block makes
it reusable but does not return its pages to the PMM. Heap operations are not
thread-safe and must not run concurrently or from interrupt handlers.
