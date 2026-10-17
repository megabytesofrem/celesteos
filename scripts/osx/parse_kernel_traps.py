#!/usr/bin/env python

# This script parses kernel trap bytes and prints a table of trap information.
# 
# Usage: Place kernel_symbols.txt and trap_bytes.txt in the same directory as this script and run it.
# NOTE: This script is written to work on both Python 2 and Python 3.

import sys

if sys.version_info[0] < 3:
    # python 2: import __future__ to provide print_function
    from __future__ import print_function
else:
    pass

symbols = {}

# Whether traps with _kern_invalid function pointer or 0x00 should be shown
filter_invalid = True

# Load symbols from the kernel_symbols.txt file
with open("kernel_symbols.txt", "r") as f:
    for line in f:
        parts = line.strip().split()
        if len(parts) == 3:
            addr, _, name = parts
            symbols[name] = int(addr, 16)

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

# Read 16 byte mach_trap_t entries
# ------------------------------------------
# arg_count is the first word (4 bytes) of the mach_trap_t structure
# func_ptr is the second word (4 bytes) of the mach_trap_t structure

# typedef struct {
#     int             mach_trap_arg_count;       /* Number of arguments */
#     int             (*mach_trap_function)(void); /* Pointer to kernel function */
# #if defined(__i386__)
#     boolean_t       mach_trap_stack;           /* Arguments on stack? (32-bit legacy) */
# #else
#     mach_munge_t    *mach_trap_arg_munge32;    /* 32-bit argument munger */
#     mach_munge_t    *mach_trap_arg_munge64;    /* 64-bit argument munger */
# #endif
# #if !MACH_ASSERT
#     int             mach_trap_unused;
# #else
#     const char      *mach_trap_name;           /* Debug name for assertions */
# #endif
# } mach_trap_t;
# ------------------------------------------
for i in range(0, len(raw_words) - 1, 2):
    trap_idx = i // 2

    # Mach syscalls are negative e.g -65 corresponds to _mach_reply_port
    syscall_num = -trap_idx

    arg_count = raw_words[i] & 0xFF # argument count is stored in lower 4 bytes of word 1
    func_ptr = raw_words[i + 1] # function pointer is the second word (4 bytes) of mach_trap_t

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