<img src="./resources/readme_logo.png">
<hr>

celesteOS is an open source operating system aiming for binary compatibility with Mac OS X 10.6 Snow Leopard at the _kernel level_. It is a Darwin-like operating system, and serves as a clean room implementation of the Darwin kernel.

## What Mac OS X version is supported?
celesteOS aims for binary compatibility with Mac OS X 10.6 Snow Leopard because modern
macOS is a moving target with frequent changes, and complex security mechanisms.

## Building
celesteOS depends on a cross compiler for `x86_64-elf` and `limine`. Both can built by running `./scripts/build_limine.sh` and
`./scripts/build_toolchain.sh`. You will need to install the packages for `nasm, qemu, xorriso`.

To build celesteOS:
```sh
make all

# To run in QEMU (requires QEMU in $PATH)
make qemu
```

Kernel builds track C header dependencies automatically. After moving source files,
update `C_SRCS` in the Makefile so the new paths are compiled into the kernel.

QEMU exposes a remote GDB server on `127.0.0.1:1234` and starts with the CPU paused.
Connect from another terminal:

```sh
gdb build/celesteos.elf
```

```gdb
target remote 127.0.0.1:1234
stepi
```

Use `stepi` to execute one instruction at a time. Execution remains paused between
steps; `continue` resumes normal execution.

## Kernel IPC objects

Both `ipc_port_t` and the separate `mach_port_t` structure embed `ipc_object_t`
as their first member. The common header owns their
lock, reference count, active bit, and object/kernel-object type bits. IPC-space
table entries refer to this header. `struct mach_port` remains a distinct
structure, and `struct mach_kmsg` remains the message container defined in
`mach/mach_port.h` and shared by both port headers; messages are not IPC objects.
Tasks, threads, and IPC spaces retain their own locks; they are not IPC objects.
Port sets are not implemented yet.

Use the object lock when accessing shared object state. The `io_reference` and
`io_release` macros require that lock; `ipc_object_reference` and
`ipc_object_release` acquire it internally. Callers must own a live reference
before retaining or accessing an object and detach type-specific resources
(including queued messages) before its final release.

IPC-space insertion/lookup and object/name allocation are still unfinished.
`ipc_port_alloc` reports failure until name allocation is implemented instead of
returning success without a usable port.

Run the host-side object/port regression checks with `make test-ipc`.

## Building user-space
celesteOS relies on a user-space replicating a native Mac OS X filesystem layout.
```
├── Applications
├── Library
├── private
│   ├── etc
│   ├── tmp
│   └── var
├── System
│   └── Library
│       ├── CoreServices
│       ├── Extensions
│       ├── Frameworks
│       └── LaunchDaemons
├── usr
│   ├── bin
│   ├── lib
│   └── libexec
│       └── launchd
└── Volumes
```

To build the user-space ramdisk, run `scripts/build_initrd.sh`. 

All binaries **must** be x86_64 native Mac OS X binaries compiled with `clang` for `x86_64-apple-darwin`. ELF is not supported, so you cannot compile a binary for celesteOS using standard ELF toolchains.

A wrapper script is provided to cross-compile binaries: `scripts/build_macho.sh <source_file> <output_file>`.