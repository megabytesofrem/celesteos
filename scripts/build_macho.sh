#!/usr/bin/env bash
set -euo pipefail

# Check arguments
if [ "$#" -ne 2 ]; then
    echo "Usage: $0 <source.c> <output_binary>"
    exit 1
fi

SOURCE_FILE="$1"
OUTPUT_FILE="$2"

# Target: x86_64 Snow Leopard (OS X 10.6) using LLD
# -ffreestanding: No host OS main() or host runtime assumptions
# -nostdlib: Don't link standard libraries or crt1.o
# -fuse-ld=lld: Use LLVM's LLD linker
# -Wl,-e,__start: Match the exact mangled `_start` C symbol in Mach-O
# -Wl,-no_pie: Standard fixed virtual memory address layout
clang --target=x86_64-apple-darwin10 \
    -ffreestanding \
    -nostdlib \
    -fuse-ld=lld \
    -Wl,-e,_start \
    -Wl,-no_pie \
    "$SOURCE_FILE" -o "$OUTPUT_FILE"