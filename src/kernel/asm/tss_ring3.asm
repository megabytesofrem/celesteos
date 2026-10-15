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

    mov ax, 0x1B    ; load user data segment selector into ax
    mov ds, ax      ; set data segment registers for user mode
    mov es, ax
    mov fs, ax
    mov gs, ax

    push 0x1B       ; push user data segment selector onto stack (SS)
    push rsi        ; push user stack pointer onto stack (RSP)
    push 0x202      ; push RFLAGS with interrupts enabled (IF = 1)
    push 0x23       ; push user code segment selector onto stack (CS)
    push rdi       ; push entry point (RIP) for user mode

    ; CPU is now in user mode
    iretq           ; return to user mode
