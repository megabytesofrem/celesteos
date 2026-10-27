[bits 64]
section .text

global ctx_switch

; -----------------------------------------------------------------------------------
; ctx_switch - Thread context switch routine
;
; Arguments:
;   rdi - Pointer to current thread's stack pointer storage (RSP)
;   rsi - Next thread's stack pointer
;
; Behaviour:
;   Saves the current thread's callee-saved registers onto the stack
;   Stores the current thread's stack pointer into the location pointed by rdi
;   Loads the next thread's stack pointer from the location pointed by rsi
;   Restores the next thread's callee-saved registers from the stack
;   Returns to the next thread's execution context
; -----------------------------------------------------------------------------------
ctx_switch: 
    ; Save current threads callee-saved registers
    push rbp
    push rbx
    push r12
    push r13
    push r14
    push r15

    ; Save current thread's stack pointer into the location pointed by rdi
    mov [rdi], rsp

    ; Load next thread's stack pointer from the location pointed by rsi
    mov rsp, rsi
    
    ; Restore next thread's callee-saved registers
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp

    ; Return to the next thread's execution context
    ret