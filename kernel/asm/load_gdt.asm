[bits 64]
section .text

global load_gdt
global load_tss

; -----------------------------------------------------------------------------------
; load_gdt - Load the Global Descriptor Table (GDT) and update segment registers
;
; Arguments:
;   RDI - Pointer to the GDT descriptor
;   DX  - Data segment selector
;   RSI - Code segment selector
;
; Behaviour:
;   Loads the GDT from the pointer in RDI
;   Updates the data segment registers with DX
;   Reloads the code segment register with RSI
; -----------------------------------------------------------------------------------
load_gdt:
    lgdt [rdi]                  ; load GDT pointer from the address in RDI

    ; Update data segment registers by loading the selector from DX
    mov ds, dx
    mov es, dx
    mov fs, dx
    mov gs, dx
    mov ss, dx

    ; Reload code segment register
    push rsi                    ; push code segment selector
    lea rax, [rel .reload_cs]   ; load address of the reload_cs label into RAX
    push rax                    ; push target return address
    retfq                       ; far return reloads CS and jumps to .reload_cs
.reload_cs:
    ; Continue execution after reloading CS
    ret
    
; -----------------------------------------------------------------------------------
; Load the Task State Segment (TSS) with the given segment selector in RDI
;
; Arguments:
;   RDI - TSS segment selector 
;
; Behaviour:
;   Loads the TSS with the segment selector provided in rdi
;   Calls ltr with the lower 16 bits of RDI (DI) to load the task register with the TSS
; -----------------------------------------------------------------------------------
load_tss:
    ltr di                      ; ltr only takes the lower 16 bits of RDI to load the task register
    ret