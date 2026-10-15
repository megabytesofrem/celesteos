[bits 64]
section .text

global _start
extern kmain

_start:
    ; Clear direction flag (AMD64 ABI requirement)
    cld

    ; Call kmain from kmain.cpp
    call kmain

.hang:
    cli
    hlt
    jmp .hang