[bits 64]
section .text

global tss_switch_ring3

; Switch from kernel mode to user mode (ring 3) using the TSS and iretq
;
; Arguments:
;   rdi - Entry point for user mode (RIP)
;   rsi - User stack pointer (RSP)
tss_switch_ring3:
    cli             ; clear interrupts during switch

    mov ax, 0x23    ; User data segment selector (DS, ES, FS, GS)
    mov ds, ax      ; Set data segment registers for user mode
    mov es, ax
    mov fs, ax
    mov gs, ax

    push 0x23       ; Push user data segment selector onto stack (SS)
    push rsi        ; Push user stack pointer onto stack (RSP)
    push 0x202      ; Push RFLAGS with interrupts enabled (IF = 1)
    push 0x1B       ; Push user code segment selector onto stack (CS)
    push rdi        ; Push entry point (RIP) for user mode

    ; Clear registers
    xor rax, rax
    xor rbx, rbx
    xor rcx, rcx
    xor rdx, rdx
    xor r8, r8
    xor r9, r9
    xor r10, r10
    xor r11, r11
    xor r12, r12
    xor r13, r13
    xor r14, r14
    xor r15, r15

    ; CPU is now in user mode
    iretq           ; return to user mode
