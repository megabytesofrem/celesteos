[bits 64]
section .text

%include "common.inc"

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

%macro DEFINE_ISR 1
isr%1:
  cli
%if %1 != 8 && %1 != 10 && %1 != 11 && %1 != 12 && %1 != 13 && %1 != 14 && %1 != 17 && %1 != 21 && %1 != 29 && %1 != 30
  push byte 0
%endif
  push byte %1
  jmp isr_common_stub
%endmacro

%macro DEFINE_IRQ 2
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
DEFINE_ISR 0        ; Divide by zero exception
DEFINE_ISR 1        ; Debug exception
DEFINE_ISR 2        ; Non-maskable interrupt
DEFINE_ISR 3        ; Breakpoint exception
DEFINE_ISR 4        ; Overflow exception
DEFINE_ISR 5        ; Bound range exceeded exception
DEFINE_ISR 6        ; Invalid opcode exception
DEFINE_ISR 7        ; Device not available exception
DEFINE_ISR 8        ; Double fault exception
DEFINE_ISR 9        ; Coprocessor segment overrun
DEFINE_ISR 10       ; Invalid TSS exception
DEFINE_ISR 11       ; Segment not present
DEFINE_ISR 12       ; Stack segment fault
DEFINE_ISR 13       ; General protection fault
DEFINE_ISR 14       ; Page fault
DEFINE_ISR 15       ; Reserved
DEFINE_ISR 16       ; x87 FPU floating-point error
DEFINE_ISR 17       ; Alignment check
DEFINE_ISR 18       ; Machine check
DEFINE_ISR 19       ; SIMD floating-point exception
DEFINE_ISR 20       ; Virtualization exception
DEFINE_ISR 21       ; Security exception
DEFINE_ISR 22       ; Reserved
DEFINE_ISR 23       ; Reserved
DEFINE_ISR 24       ; Reserved
DEFINE_ISR 25       ; Reserved
DEFINE_ISR 26       ; Reserved
DEFINE_ISR 27       ; Reserved
DEFINE_ISR 28       ; Reserved
DEFINE_ISR 29       ; Reserved
DEFINE_ISR 30       ; Reserved
DEFINE_ISR 31       ; Reserved

; Define the IRQs
DEFINE_IRQ 0, 32   ; Timer interrupt
DEFINE_IRQ 1, 33   ; Keyboard interrupt
DEFINE_IRQ 2, 34   ; Cascade interrupt (used internally by the PIC)
DEFINE_IRQ 3, 35   ; COM2 serial port
DEFINE_IRQ 4, 36   ; COM1 serial port
DEFINE_IRQ 5, 37   ; LPT2 parallel port
DEFINE_IRQ 6, 38   ; Floppy disk controller
DEFINE_IRQ 7, 39   ; LPT1 parallel port
DEFINE_IRQ 8, 40   ; Real-time clock
DEFINE_IRQ 9, 41   ; ACPI interrupt
DEFINE_IRQ 10, 42  ; Reserved
DEFINE_IRQ 11, 43  ; Reserved
DEFINE_IRQ 12, 44  ; PS/2 mouse
DEFINE_IRQ 13, 45  ; FPU / Coprocessor / Inter-processor
DEFINE_IRQ 14, 46  ; Primary ATA hard disk
DEFINE_IRQ 15, 47  ; Secondary ATA hard disk