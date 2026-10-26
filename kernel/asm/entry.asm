[bits 64]
section .text

global _start
extern kmain

_start:
    cli          ; Clear interrupts
    cld          ; Clear direction flag

    and rsp, -16 ; Align stack to 16-byte boundary
    call kmain

.hang:
    cli
    hlt
    jmp .hang
