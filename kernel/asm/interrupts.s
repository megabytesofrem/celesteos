[bits 64]
section .text

global isr0 ; Divide by zero exception
global isr1 ; Debug exception
global isr2 ; Non-maskable interrupt
global isr3 ; Breakpoint exception
global isr4 ; Overflow exception
global isr5 ; Bound range exceeded exception
global isr6 ; Invalid opcode exception
global isr7 ; Device not available exception
global isr8 ; Double fault exception
global isr9 ; Coprocessor segment overrun
global isr10 ; Invalid TSS exception
global isr11 ; Segment not present
global isr12 ; Stack-segment fault
global isr13 ; General protection fault
global isr14 ; Page fault
global isr15 ; Reserved
global isr16 ; x87 FPU floating-point error
global isr17 ; Alignment check
global isr18 ; Machine check
global isr19 ; SIMD floating-point exception
global isr20 ; Virtualization exception
global isr21 ; Control protection exception
global isr22 ; Reserved
global isr23 ; Reserved
global isr24 ; Reserved
global isr25 ; Reserved
global isr26 ; Reserved
global isr27 ; Reserved
global isr28 ; Reserved
global isr29 ; Reserved
global isr30 ; Reserved
global isr31 ; Reserved

; IRQs
global irq0  ; PIC timer interrupt
global irq1  ; Keyboard (PS/2)
global irq2  ; Cascade (used internally by the PIC)
global irq3  ; COM2 (or COM4)
global irq4  ; COM1 (or COM3)
global irq5  ; LPT2 (or sound card)
global irq6  ; Floppy disk controller
global irq7  ; LPT1 / spurious interrupt
global irq8  ; Real-time clock
global irq9  ; ACPI
global irq10 ; Available / SCSI / NIC
global irq11 ; Available / SCSI / NIC
global irq12 ; PS/2 mouse
global irq13 ; FPU / Coprocessor / Inter-processor
global irq14 ; Primary ATA hard disk
global irq15 ; Secondary ATA hard disk

extern isr_handler
extern irq_handler

%macro pushall 0
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
%endmacro

%macro popall 0
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
%endmacro

%macro define_isr 1
isr%1:
  cli
%if %1 != 8 && %1 != 10 && %1 != 11 && %1 != 12 && %1 != 13 && %1 != 14 && %1 != 17 && %1 != 21 && %1 != 29 && %1 != 30
  push byte 0
%endif
  push byte %1
  jmp isr_common_stub
%endmacro

%macro define_irq 2
irq%1:
  push byte 0
  push byte %2
  jmp irq_common_stub
%endmacro

isr_common_stub:
  pushall           ; push all registers
  mov rax, cr2
  push rax
  cld               ; clear direction flag
  mov rdi, rsp      ; first C argument: saved register frame
  mov rbp, rsp      ; preserve frame pointer across the C call
  and rsp, -16      ; System V AMD64 call alignment
  call isr_handler  ; call the common ISR handler
  mov rsp, rbp
  add rsp, 8        ; discard saved CR2
  popall            ; restore all registers

  add rsp, 16       ; remove error code and interrupt number from stack
  iretq             ; return from interrupt

irq_common_stub:
  pushall           ; push all registers
  mov rax, cr2
  push rax
  cld               ; clear direction flag
  mov rdi, rsp
  mov rbp, rsp
  and rsp, -16
  call irq_handler  ; call the common IRQ handler
  mov rsp, rbp
  add rsp, 8
  popall            ; restore all registers

  add rsp, 16       ; remove error code and interrupt number from stack
  iretq             ; return from interrupt

; Define the ISRs
define_isr 0        ; Divide by zero exception
define_isr 1        ; Debug exception
define_isr 2        ; Non-maskable interrupt
define_isr 3        ; Breakpoint exception
define_isr 4        ; Overflow exception
define_isr 5        ; Bound range exceeded exception
define_isr 6        ; Invalid opcode exception
define_isr 7        ; Device not available exception
define_isr 8        ; Double fault exception
define_isr 9        ; Coprocessor segment overrun
define_isr 10       ; Invalid TSS exception
define_isr 11       ; Segment not present
define_isr 12       ; Stack segment fault
define_isr 13       ; General protection fault
define_isr 14       ; Page fault
define_isr 15       ; Reserved
define_isr 16       ; x87 FPU floating-point error
define_isr 17       ; Alignment check
define_isr 18       ; Machine check
define_isr 19       ; SIMD floating-point exception
define_isr 20       ; Virtualization exception
define_isr 21       ; Security exception
define_isr 22       ; Reserved
define_isr 23       ; Reserved
define_isr 24       ; Reserved
define_isr 25       ; Reserved
define_isr 26       ; Reserved
define_isr 27       ; Reserved
define_isr 28       ; Reserved
define_isr 29       ; Reserved
define_isr 30       ; Reserved
define_isr 31       ; Reserved

; Define the IRQs
define_irq 0, 32   ; Timer interrupt
define_irq 1, 33   ; Keyboard interrupt
define_irq 2, 34   ; Cascade interrupt (used internally by the PIC)
define_irq 3, 35   ; COM2 serial port
define_irq 4, 36   ; COM1 serial port
define_irq 5, 37   ; LPT2 parallel port
define_irq 6, 38   ; Floppy disk controller
define_irq 7, 39   ; LPT1 parallel port
define_irq 8, 40   ; Real-time clock
define_irq 9, 41   ; ACPI interrupt
define_irq 10, 42  ; Reserved
define_irq 11, 43  ; Reserved
define_irq 12, 44  ; PS/2 mouse
define_irq 13, 45  ; FPU / Coprocessor / Inter-processor
define_irq 14, 46  ; Primary ATA hard disk
define_irq 15, 47  ; Secondary ATA hard disk