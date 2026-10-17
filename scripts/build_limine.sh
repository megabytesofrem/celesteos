#!/usr/bin/env bash

set -euo pipefail

# Usage: ./build_limine.sh [make arguments, e.g. -j4]
# Set LIMINE_DIR to use a different build directory.
# The binary branch supplies boot files; make builds the host installer.
ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd -- "$ROOT/.." && pwd)"
LIMINE_DIR="${LIMINE_DIR:-$ROOT/build/limine}"
LIMINE_BRANCH=v11.x-binary

if [[ ! -e "$LIMINE_DIR" ]]; then
    mkdir -p "$(dirname -- "$LIMINE_DIR")"
    git clone --depth=1 --branch="$LIMINE_BRANCH" --single-branch \
        https://github.com/limine-bootloader/limine.git "$LIMINE_DIR"
fi

if [[ ! -f "$LIMINE_DIR/Makefile" || ! -f "$LIMINE_DIR/limine.c" ]]; then
    printf 'Error: %s is not a Limine binary checkout.\n' "$LIMINE_DIR" >&2
    exit 1
fi

if command -v gmake >/dev/null 2>&1; then
    MAKE=gmake
else
    MAKE=make
fi

"$MAKE" -C "$LIMINE_DIR" "$@"
for file in limine limine-bios.sys limine-bios-cd.bin; do
    if [[ ! -f "$LIMINE_DIR/$file" ]]; then
        printf 'Error: required Limine artifact is missing: %s\n' "$LIMINE_DIR/$file" >&2
        exit 1
    fi
done

# Build and stage the initrd where limine.conf expects its module.
"$BASH" "$ROOT/build_initrd.sh"
ISO_BOOT_DIR="$PROJECT_ROOT/build/iso_root/boot"
mkdir -p "$ISO_BOOT_DIR"
cp "$ROOT/build/initrd.tar" "$ISO_BOOT_DIR/initrd.tar"

printf 'Limine boot files and host installer are in %s\n' "$LIMINE_DIR"
printf 'Initrd is staged in %s\n' "$ISO_BOOT_DIR"
