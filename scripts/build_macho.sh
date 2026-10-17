#!/usr/bin/env bash
set -euo pipefail

# Link a freestanding x86_64 Mach-O executable with the user program's _start entry point.
clang --target=x86_64-apple-macosx10.15 \
    -ffreestanding -nostdlib -fuse-ld=lld \
    -Wl,-e,__start -Wl,-no_pie \
    "$1" -o "$2"