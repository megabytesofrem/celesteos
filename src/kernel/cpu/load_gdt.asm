[bits 64]
section .text

; Make this global so we can use it in C/C++
global load_gdt

; Load the Global Descriptor Table (GDT) and update segment registers
; Arguments:
;   rdi - Pointer to the GDT descriptor
;   dx  - Data segment selector
;   rsi - Code segment selector
load_gdt:
    lgdt [rdi]                  ; load GDT pointer from the address in rdi

    ; Update data segment registers
    mov ds, dx
    mov es, dx
    mov fs, dx
    mov gs, dx
    mov ss, dx

    ; Reload code segment register
    push rsi                    ; push code segment selector
    lea rax, [rel .reload_cs]
    push rax                    ; push target return address
    retfq                       ; far return reloads CS and jumps to .reload_cs

.reload_cs:
    ; Continue execution after reloading CS
    ret
    