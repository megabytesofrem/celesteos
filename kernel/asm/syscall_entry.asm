[bits 64]
global syscall_entry
extern mach_syscall_dispatch

section .bss
user_rsp:           resq 1 ; user mode stack pointer
global syscall_kernel_stack_top
syscall_kernel_stack_top: resq 1 ; top of the kernel stack

section .text
syscall_entry:
    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    ; CPU saved RIP in RCX and user RFLAGS in R11
    ; RSP still points to the usermode stack
    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

    ; Save the current user mode stack pointer
    mov [rel user_rsp], rsp
    ; Switch to the kernel stack
    mov rsp, [rel syscall_kernel_stack_top]

    ; Preserve registers on kernel stack by pushing them
    ; x86 stack grows downwards, so we push registers in reverse order (e.g r11, then rcx)

    push r11    ; holds user RFLAGS
    push rcx    ; holds user RIP
    push qword [rel user_rsp]
    push rbp
    push r15
    push r14
    push r13
    push r12
    push r11
    push r10
    push r9
    push r8
    push rdx
    push rsi
    push rdi
    push rbx
    push rax    ; rax holds the syscall number

    ; Pass syscall number and register frame in rax and rsp to the syscall dispatcher
    mov rdi, rax    ; first argument: syscall number
    mov rsi, rsp    ; second argument: pointer to the saved register frame
    sub rsp, 8     ; align the stack for the System V AMD64 C call
    call mach_syscall_dispatch
    add rsp, 8

    ; Restore registers from the kernel stack
    ; x86 stack grows downwards, so we must pop in the reverse order of pushes
    add rsp, 8 ; Discard the saved syscall number; keep the dispatcher result in RAX
    pop rbx
    pop rdi
    pop rsi
    pop rdx
    pop r8
    pop r9
    pop r10
    pop r11
    pop r12
    pop r13
    pop r14
    pop r15
    pop rbp
    add rsp, 8  ; Discard the saved user RSP
    pop rcx    ; Restores User RIP
    pop r11    ; Restores User RFLAGS

    ; Return to user mode with the existing GDT's user selectors.
    push qword 0x23
    push qword [rel user_rsp]
    push r11
    push qword 0x1B
    push rcx
    iretq