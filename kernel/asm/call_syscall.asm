[bits 64]
global call_syscall
extern sys_dispatch

section .bss
user_stack: resq 1           ; user stack pointer
global syscall_kernel_stack
syscall_kernel_stack: resq 1 ; kernel stack pointer

section .text

; ------------------------------------------------------------------------------------------
; push_frame / pop_frame - Macros to save and restore CPU state on the kernel stack
; ------------------------------------------------------------------------------------------
%macro push_frame 0
    ; Preserve registers on kernel stack by pushing them in inverse order
    ; as x86 grows downwards
    push r11                            ; R11 holds user RFLAGS
    push rcx                            ; RCX holds user return address (RIP)
    push qword [rel user_stack]         ; save user stack pointer by pushing 16 bytes (quadword)
    push rbp                            ; save base pointer
    push r15
    push r14
    push r13
    push r12

    ; It looks cursed to skip r11, but we already saved it at the top of the stack
    push r10
    push r9
    push r8
    push rdx
    push rsi
    push rdi
    push rbx
    push rax                            ; RAX holds the syscall number
%endmacro

%macro pop_frame 0
    ; Restore registers from kernel stack in reverse order
    pop rax                             ; consume saved RAX (or pop rbx if skipping RAX restore)
    pop rbx
    pop rdi
    pop rsi
    pop rdx
    pop r8
    pop r9
    pop r10
    pop r12                             ; matched push order (r12 comes before r13)
    pop r13
    pop r14
    pop r15
    pop rbp
    pop qword [rel user_stack]          ; restore user stack pointer
    pop rcx                             ; restore user return address (RIP)
    pop r11                             ; restore user RFLAGS
%endmacro

; ------------------------------------------------------------------------------------------
; call_syscall - Entry point for handling system calls from user mode
;
; Arguments:
;   rax - syscall number
;   rdi, rsi, rdx, r10, r8, r9 - syscall arguments (if any)
;
; Behaviour:
;   Saves the current user stack pointer into user_stack and switches to the kernel stack (rsp = syscall_kernel_stack).
;   Pushes all general purpose registers (x86 grows downwards, so inverse order is used) to build a CPU frame
;   Calls sys_dispatch with the syscall number and a pointer to the CPU frame
;   Restores all registers and returns to user mode using iretq
; ------------------------------------------------------------------------------------------
call_syscall:
    ; Save the current user stack pointer
    mov [rel user_stack], rsp
    ; Switch to the kernel stack
    mov rsp, [rel syscall_kernel_stack]

    ; Build the CPU frame on the kernel stack
    push_frame
    
    ; Pass syscall arguments in registers as per the calling convention
    mov rdi, rax                        ; first argument: syscall number
    mov rsi, rsp                        ; second argument: syscall argument 1 (svc_cpu_frame_t*)
    call sys_dispatch                   ; perform the system call, trashes rcx with rip, r11 with rflags
                                        ; rax holds the return value from the syscall

    ; Restore stack pointer after syscall
    pop_frame


    

    ; Return to user mode with the existing GDT segment selectors
    push qword 0x23                     ; user data segment selector
    push qword [rel user_stack]         ; user stack pointer
    push r11                            ; user RFLAGS
    push qword 0x1B                     ; user code segment selector
    push rcx                            ; user return address (RIP)
    iretq                               ; return to user mode