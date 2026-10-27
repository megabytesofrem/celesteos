[bits 64]
global call_syscall

; klog is defined externally in C
extern klog

; Service call handlers are defined externally in C
extern sys_svc_send_msg
extern sys_svc_reply_wait
extern sys_svc_vm_map
extern sys_svc_vm_unmap
extern sys_svc_thread_create
extern sys_svc_thread_yield

; ------------------------------------------------------------------------------------------
; Logging levels
; ------------------------------------------------------------------------------------------
%define KLOG_NONE           0
%define KLOG_INFO           1

; ------------------------------------------------------------------------------------------
; Service call numbers
; ------------------------------------------------------------------------------------------    
%define SVC_SEND_MSG        0
%define SVC_REPLY_WAIT      1
%define SVC_VM_MAP          2
%define SVC_VM_UNMAP        3
%define SVC_THREAD_CREATE   4
%define SVC_THREAD_YIELD    5

struc svc_cpu_frame
    .r15:    resq 1
    .r14:    resq 1
    .r13:    resq 1
    .r12:    resq 1
    .r11:    resq 1
    .r10:    resq 1
    .r9:     resq 1
    .r8:     resq 1
    .rbp:    resq 1
    .rdi:    resq 1
    .rsi:    resq 1
    .rdx:    resq 1
    .rcx:    resq 1
    .rbx:    resq 1
    .rax:    resq 1
    .rip:    resq 1
    .cs:     resq 1
    .rflags: resq 1
    .rsp:    resq 1
    .ss:     resq 1
endstruc

; ------------------------------------------------------------------------------------------
section .bss
user_stack:                 resq 1           ; user stack pointer

global syscall_kernel_stack
syscall_kernel_stack:       resq 1           ; kernel stack pointer

; ------------------------------------------------------------------------------------------
section .data
dispatch_msg:               db "svc: Dispatching service call: %d", 0

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

%macro log 3
    mov rdi, %1
    lea rsi, [rel %2]
    mov rdx, %3
    call klog
%endmacro

; ------------------------------------------------------------------------------------------
; call_syscall - Entry point for handling system calls from user mode
;
; Arguments:
;   RAX - syscall number
;   RDI, RSI, RDX, R10, R8, R9 - syscall arguments (if any)
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
    mov rbx, rsp                        ; save the CPU frame pointer in rbx for later use
    and rsp, -16                        ; align the stack pointer to a 16-byte boundary


    call sys_dispatch                   ; call the dispatcher, trashes rcx with rip, r11 with rflags
                                        ; rax holds the return value from the syscall
    mov rsp, rbx                        ; restore the CPU frame pointer to rsp after syscall

    ; Restore stack pointer after syscall
    pop_frame

    ; Return to user mode with the existing GDT segment selectors
    push qword 0x23                     ; user data segment selector
    push qword [rel user_stack]         ; user stack pointer
    push r11                            ; user RFLAGS
    push qword 0x1B                     ; user code segment selector
    push rcx                            ; user return address (RIP)
    iretq                               ; return to user mode

; ------------------------------------------------------------------------------------------
; sys_dispatch - Dispatch system calls to their C handlers.
;
; Notes:
;   1: Prior to calling this function, the stack pointer has been aligned to a 16 byte boundary,
;   since the previous procedure `call_syscall` did that for us before invoking this function.
;
;   2: klog and sys_svc_xxxx procedures are defined externally in C.
;
; Arguments:
;   RDI - first syscall argument (service call number)
;   RSI - rest are passed in a CPU frame structure (svc_cpu_frame_t*)
; ------------------------------------------------------------------------------------------
sys_dispatch:
    push rdi                            ; save the first syscall argument on the stack
    push rsi                            ; save the CPU frame pointer on the stack

    ; klog(LOG_INFO, "svc: Dispatching service call: %d", syscall_number)
    log KLOG_INFO, dispatch_msg, rdi

    pop rsi                            ; restore the CPU frame pointer
    pop rdi                            ; restore the first syscall argument

    ; Handle the service call
    cmp r11, SVC_SEND_MSG
    je .svc_send_msg                    ; dispatch to SVC_SEND_MSG
    cmp r11, SVC_REPLY_WAIT
    je .svc_reply_wait                  ; dispatch to SVC_REPLY_WAIT
    cmp r11, SVC_VM_MAP
    je .svc_vm_map                      ; dispatch to SVC_VM_MAP
    cmp r11, SVC_VM_UNMAP
    je .svc_vm_unmap                    ; dispatch to SVC_VM_UNMAP
    cmp r11, SVC_THREAD_CREATE
    je .svc_thread_create               ; dispatch to SVC_THREAD_CREATE
    cmp r11, SVC_THREAD_YIELD
    je .svc_thread_yield                ; dispatch to SVC_THREAD_YIELD

    ; Unknown syscall handler
    jne .svc_unknown                      ; dispatch to unknown syscall handler
.svc_send_msg:
    mov r11, rsi                        ; save the CPU frame pointer in r11 for later use
    mov rdi, [r11 + svc_cpu_frame.rdi]  ; rdi = svc_cpu_frame->rdi
    mov rsi, [r11 + svc_cpu_frame.rsi]  ; rsi = svc_cpu_frame->rsi
    mov rdx, [r11 + svc_cpu_frame.rdx]  ; rdx = svc_cpu_frame->rdx
    call sys_svc_send_msg
    ret
.svc_reply_wait:
    mov r11, rsi                        ; save the CPU frame pointer in r11 for later use
    mov rdi, [r11 + svc_cpu_frame.rdi]  ; rdi = svc_cpu_frame->rdi
    mov rsi, [r11 + svc_cpu_frame.rsi]  ; rsi = svc_cpu_frame->rsi
    mov rdx, [r11 + svc_cpu_frame.rdx]  ; rdx = svc_cpu_frame->rdx
    call sys_svc_reply_wait
    ret
.svc_vm_map:
    mov r11, rsi                        ; save the CPU frame pointer in r11 for later use
    mov rdi, [r11 + svc_cpu_frame.rdi]  ; rdi = svc_cpu_frame->rdi
    mov rsi, [r11 + svc_cpu_frame.rsi]  ; rsi = svc_cpu_frame->rsi
    mov rdx, [r11 + svc_cpu_frame.rdx]  ; rdx = svc_cpu_frame->rdx
    mov r8, [r11 + svc_cpu_frame.r8]    ; r8 = svc_cpu_frame->r8
    call sys_svc_vm_map
    ret
.svc_vm_unmap:
    mov r11, rsi                        ; save the CPU frame pointer in r11 for later use
    mov rdi, [r11 + svc_cpu_frame.rdi]  ; rdi = svc_cpu_frame->rdi
    mov rsi, [r11 + svc_cpu_frame.rsi]  ; rsi = svc_cpu_frame->rsi
    call sys_svc_vm_unmap
    ret
.svc_thread_create:
    mov r11, rsi                        ; save the CPU frame pointer in r11 for later use
    mov rdi, [r11 + svc_cpu_frame.rdi]  ; rdi = svc_cpu_frame->rdi
    mov rsi, [r11 + svc_cpu_frame.rsi]  ; rsi = svc_cpu_frame->rsi
    mov rdx, [r11 + svc_cpu_frame.rdx]  ; rdx = svc_cpu_frame->rdx
    call sys_svc_thread_create
    ret
.svc_thread_yield:
    mov r11, rsi                        ; save the CPU frame pointer in r11 for later use
    mov rdi, [r11 + svc_cpu_frame.rdi]  ; rdi = svc_cpu_frame->rdi
    call sys_svc_thread_yield
    ret
.svc_unknown:
    mov r11, rsi                        ; save the CPU frame pointer in r11 for later use
    mov rdi, [r11 + svc_cpu_frame.rdi]  ; rdi = svc_cpu_frame->rdi
    call sys_svc_unknown
    ret