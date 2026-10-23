<img src="./resources/readme_logo.png">
<hr>

celesteOS is an open source operating system aiming for binary compatibility with Mac OS X 10.6 Snow Leopard at the _kernel level_. It is a Darwin-like operating system, and serves as a clean room implementation of the Darwin kernel.

## What Mac OS X version is supported?
celesteOS aims for binary compatibility with Mac OS X 10.6 Snow Leopard because modern
macOS is a moving target with frequent changes, and complex security mechanisms.

## Building
celesteOS depends on a cross compiler for `x86_64-elf` and `limine`. Both can built by running `./scripts/build_limine.sh` and `./scripts/build_toolchain.sh`.

You will also need the following packages: `ruby`, `clang`, `nasm`, `qemu`,
`xorriso`, and `e2fsprogs` (for `mke2fs`).

To build celesteOS:
```sh
make all

# To run in QEMU (requires QEMU in $PATH)
make qemu
```

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

## Project Structure
```text
kernel/            Kernel implementation, architecture code, and ABI interfaces
lib/stdlib/        Freestanding support library used by the kernel
include/           Shared external protocol headers
userspace/
  programs/        User program C sources
  rootfs/          Hand-maintained root filesystem skeleton and configuration
build/             Generated objects, binaries, ISO tree, rootfs, and initrd
scripts/           Build and utility scripts
tests/             Host-side tests
resources/         Images and other project assets
```

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

To build the user-space ramdisk, run `make initrd`. The generated filesystem is
staged in `build/rootfs/`, and the resulting 32 MiB ext2 image is
`build/initrd.ext2`. `make iso` builds and includes this image automatically.
Limine loads it into memory as the module identified by `ext2-initrd`; no disk
driver or QEMU `-drive` option is needed.

The kernel retains the module as the ext2 backing image and performs baseline
superblock/geometry checks. It resolves `/usr/libexec/launchd`, reads its contents,
loads its Mach-O segments into a user address space, and enters ring 3.
The ext2 reader currently supports direct blocks and sparse holes only;
files or directories requiring indirect blocks are rejected. Full metadata
validation and general-purpose filesystem support remain unfinished.

### Create an ext2 image from a directory

Given an existing directory containing the desired filesystem tree:

```sh
mkdir -p build
# Creates/overwrites this disposable image; never substitute a real disk device.
truncate -s 32M build/custom.ext2
mke2fs -t ext2 -F -b 1024 -I 128 -O none \
    -d /absolute/path/to/rootfs build/custom.ext2

# Boot this image instead of rebuilding the default userspace image.
make qemu INITRD_IMAGE=build/custom.ext2
```

Use a different output filename for an image you want to keep: image creation
overwrites that file. Increase the size if the directory does not fit. The image
contains a filesystem directly, with no partition table. These explicit options
match the baseline format currently accepted by the kernel. A warning about
128-byte inode timestamps is expected from newer e2fsprogs.

All binaries **must** be x86_64 native Mac OS X binaries compiled with `clang` for `x86_64-apple-darwin`. ELF is not supported, so you cannot compile a binary for celesteOS using standard ELF toolchains.

Binaries placed in `userspace/programs/` will be compiled into Mach-O binaries and
included in the generated initrd automatically during the build process.