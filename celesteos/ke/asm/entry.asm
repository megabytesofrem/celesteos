[bits 64]
section .text

global _start
extern kmain

; --------------------------------------------------------
; _start - Entry point for the kernel
; --------------------------------------------------------
_start:
    cli          ; clear interrupts
    cld          ; clear direction flag

    and rsp, -16 ; align stack to 16-byte boundary
    call kmain   ; call kmain

.hang:
    cli
    hlt
    jmp .hang
