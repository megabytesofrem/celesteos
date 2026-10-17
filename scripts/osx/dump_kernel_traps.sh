#!/bin/bash

# This script dumps the Mach trap table from the OS X kernel.
# Run this script on a OS X 10.6 system as it will not work on anything else.
#
# Usage: ./dump_kernel_traps.sh
# Output will be saved to trap_bytes.txt, kernel_symbols.txt and kernel_segments.txt
#
# Alternatively, you may dump the files on a Mac, and then use llvm-otool to analyze them.

echo "Locating _mach_trap_table."
TRAP_TABLE_ADDR=$(nm /mach_kernel | grep "_mach_trap_table" | cut -d ' ' -f1)

echo "Found _mach_trap_table at 0x$TRAP_TABLE_ADDR within /mach_kernel."

# Dump the trap table using GDB
echo -e "set pagination off\nx/512gx 0x$TRAP_TABLE_ADDR" | gdb -batch -x /dev/stdin /mach_kernel > trap_bytes.txt

# Dump the kernel symbols
echo "Dumping kernel symbols."
nm -n /mach_kernel > kernel_symbols.txt

# Dump the kernel segments
echo "Dumping kernel segments."
otool -l /mach_kernel > kernel_segments.txt