#!/usr/bin/env python

# This script parses kernel trap bytes and prints a table of trap information.
# 
# Usage: Place kernel_symbols.txt and trap_bytes.txt in the same directory as this script and run it.
# NOTE: This script is written to work on both Python 2 and Python 3.

from __future__ import print_function
import sys

symbols = {}

# Whether traps with _kern_invalid function pointer or 0x00 should be shown
filter_invalid = True

# Load symbols from the kernel_symbols.txt file
with open("kernel_symbols.txt", "r") as f:
    for line in f:
        parts = line.strip().split()
        if len(parts) == 3:
            addr, _, name = parts
            symbols[int(addr, 16)] = name

# Load GDB trap bytes to parse
raw_words = []
with open("trap_bytes.txt", "r") as f:
    for line in f:
        if ':' in line:
            # Drop the address prefix (0xNNNNN) and split the rest into words
            words = line.split(':')[1].strip().split()
            for w in words:
                # Convert each word to an integer if it starts with "0x"
                if w.startswith("0x"):
                    raw_words.append(int(w, 16))

# Print table header
print("%-10s | %-12s | %-6s | %s" % ("Trap Index", "Syscall", "Args", "Function Handler"))
print("-" * 60)

# In 64-bit OS X 10.6 (XNU-1504), sizeof(mach_trap_t) = 32 bytes (4 x 64-bit giant words)
# raw_words[i + 0] : arg_count (int32) + padding
# raw_words[i + 1] : mach_trap_function (uint64_t pointer)
# raw_words[i + 2] : mach_trap_arg_munge32 pointer
# raw_words[i + 3] : mach_trap_arg_munge64 pointer
STRUCT_WORDS = 5

# Read 40 bytes (5 x 64-bit words) per mach_trap_t entry
# ------------------------------------------
# arg_count is the first word (4 bytes) of the mach_trap_t structure
# func_ptr is the second word (4 bytes) of the mach_trap_t structure

# typedef struct {
#     int             mach_trap_arg_count;    /* 4 bytes (+ 4 bytes compiler padding) = 8 bytes */
#     mach_trap_fn_t  mach_trap_function;     /* 8 bytes (64-bit function pointer) */
#     mach_munge_t    *mach_trap_arg_munge32; /* 8 bytes (32-bit munge pointer) */
#     mach_munge_t    *mach_trap_arg_munge64; /* 8 bytes (64-bit munge pointer) */
#     const char      *mach_trap_name;        /* 8 bytes (debug string pointer!) */
# } mach_trap_t;                              /* Total = 40 bytes! */
# ------------------------------------------
for i in range(0, len(raw_words) - STRUCT_WORDS + 1, STRUCT_WORDS):
    trap_idx = i // STRUCT_WORDS

    # Mach syscalls are negative e.g -65 corresponds to _mach_reply_port
    syscall_num = -trap_idx

    arg_count = raw_words[i] & 0xFFFFFFFF # argument count
    func_ptr = raw_words[i + 1] # function pointer

    # Resolve symbol name
    symbol_name = symbols.get(func_ptr, "0x%x" % func_ptr)

    # Print the trap information in a tabular format
    if filter_invalid:
        if symbol_name != "_kern_invalid" and symbol_name != "0x0":
            print("%-10d | %-12d | %-6d | %s" % (trap_idx, syscall_num, arg_count, symbol_name))
    else:
        print("%-10d | %-12d | %-6d | %s" % (trap_idx, syscall_num, arg_count, symbol_name))

    # Only maximum of 256 traps
    if trap_idx >= 255:
        break