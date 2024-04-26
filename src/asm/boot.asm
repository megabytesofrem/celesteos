; The entry point for the kernel, place it at high memory
%define KERNEL_HIGH_VMA 0xFFFFFFFF80000000

%macro GenPD_2MB 3
    %assign i %1
    %rep 2
        dq (i | 0x83)
        %assign i i+0x200000
    %endrep
    %rep 3
        dq 0
    %endrep
%endmacro

; Multiboot header
MB_ALIGN equ 1 << 0     ; Align modules on page boundaries
MB_MEMINFO equ 1 << 1   ; Provide memory map

FLAGS equ MB_ALIGN | MB_MEMINFO
MAGIC equ 0x1BADB002
CHECKSUM equ -(MAGIC + FLAGS)

section .multiboot
; Align the multiboot header by 4 bytes
align 4
    dd MAGIC
    dd FLAGS
    dd CHECKSUM

section .bss
align 16
stack_bottom:
    resb 16384
stack_top:

section .text
global _start

_start:
    bits 32
    
    ; The bootloader has loaded us into protected mode
    ; Interrupts are disabled, pagng is disabled, and we are running in 32-bit mode
    mov edi, ebx
    mov esp, stack_top - KERNEL_HIGH_VMA

    ; load page tables
    mov eax, pml4_table - KERNEL_HIGH_VMA
    mov cr3, eax

    ; enable PAE
    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax

    ; long mode bit, we will enter 64-bit mode
    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8
    wrmsr

    ; enable paging
    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax

    ; load the GDT
    lgdt [gdt.ptr_low - KERNEL_HIGH_VMA]
    jmp (0x8):(start64 - KERNEL_HIGH_VMA)

    bits 64

start64:
    bits 64
    mov ax, 0x10
    mov ss, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; jump to higher half kernel
    mov rax, higher_half
    jmp rax

higher_half:
    mov rsp, stack_top
    lgdt [gdt.ptr]

    extern kmain
    call kmain
    cli
.hang:
    hlt
    jmp .hang

section .data

; Define a static page table that maps 1 GB
align 4096
pml4_table:
    ; Map lower half of PML 4
    dq pml3_table_low + 0x3 - KERNEL_HIGH_VMA
    ; Pad with 510 zeros
    times 510 dq 0

    ; Map higher half of PML 4
    dq pml3_table_high + 0x3 - KERNEL_HIGH_VMA
pml3_table_low:
    ; Map lower half of Page Directory Pointer Table
    dq pml2_table + 0x3 - KERNEL_HIGH_VMA

    ; Pad with 511 zeros
    times 511 dq 0
pml3_table_high:
    ; Pad with 510 zeros
    times 510 dq 0

    ; Map higher half of Page Directory Pointer Table
    dq pml2_table + 0x3 - KERNEL_HIGH_VMA
pml2_table:
    ; Identity map 1 GB of 2 MB pages
    GenPD_2MB 0, 512, 0

; Define a static initial GDT
gdt:
.null: equ $ - gdt
    dw 0xFFFF   ; limit (low)
    dw 0        ; base (low)
    db 0        ; base (middle)
    db 0        ; access byte
    db 0        ; granularity byte
    db 0        ; base (high)

.kernel_code: equ $ - gdt
    dw 0        
    dw 0        
    db 0        
    db 10011010b; access byte (exec/read)
    db 10101111b; granularity byte
    db 0        

.kernel_data: equ $ - gdt
    dw 0        
    dw 0        
    db 0        
    db 10010010b; access byte (read/write)
    db 0        ; granularity byte
    db 0        

.user_code: equ $ - gdt
    dw 0        
    dw 0        
    db 0        
    db 11111010b; access byte (exec/read)
    db 10101111b; granularity byte
    db 0

.user_data: equ $ - gdt
    dw 0        
    dw 0        
    db 0        
    db 10010010b; access byte (read/write)
    db 0        ; granularity byte
    db 0

.ptr:
    dw $ - gdt - 1
    dq gdt
; 32 bit GDT pointer
.ptr_low:
    dw $ - gdt - 1
    dq gdt - KERNEL_HIGH_VMA

