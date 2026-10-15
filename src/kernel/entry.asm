[bits 64]
section .text

global _start
extern kmain

_start:
    ; Clear direction flag (AMD64 ABI requirement)
    cld

    ; Call C++ main entry point
    call kmain

.hang:
    cli
    hlt
    jmp .hang