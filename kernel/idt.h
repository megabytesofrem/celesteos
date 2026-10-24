#ifndef IDT_H
#define IDT_H

#include <stdint.h>

struct cpu_registers {
    uint64_t cr2;
    uint64_t r15;
    uint64_t r14;
    uint64_t r13;
    uint64_t r12;
    uint64_t r11;
    uint64_t r10;
    uint64_t r9;
    uint64_t r8;
    uint64_t rbp;
    uint64_t rdi;
    uint64_t rsi;
    uint64_t rdx;
    uint64_t rcx;
    uint64_t rbx;
    uint64_t rax;

    uint64_t int_no, err_code;
    uint64_t rip, cs, rflags, rsp, ss;
};

typedef struct __attribute__((packed)) idt_entry {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t ist;
    uint8_t type_attr;
    uint16_t offset_middle;
    uint32_t offset_high;
    uint32_t zero;
} idt_entry_t;

typedef struct __attribute__((packed)) idt_pointer_t {
    uint16_t limit; /* size of the IDT in bytes - 1 */
    uint64_t base;  /* address of the first element in the IDT */
} idt_pointer_t;

typedef void (*idt_handler_t)(void);

/**
 * Initialize the Interrupt Descriptor Table (IDT).
 */
void idt_init(void);

/**
 * Set an entry in the IDT to point to a specific interrupt handler.
 */
void idt_set_entry(int entry, idt_handler_t handler, int type_attr);

/**
 * Set an entry in the IDT to point to a specific interrupt handler with a
 * given Interrupt Stack Table (IST) index.
 */
void idt_set_entry_ist(int entry, idt_handler_t handler, int type_attr, int ist_index);

void idt_load(struct idt_pointer_t *ptr);

/* ISR */

/* IRQs are defined in Assembly */
extern void irq0();
extern void irq1();
extern void irq2();
extern void irq3();
extern void irq4();
extern void irq5();
extern void irq6();
extern void irq7();
extern void irq8();
extern void irq9();
extern void irq10();
extern void irq11();
extern void irq12();
extern void irq13();
extern void irq14();
extern void irq15();

/* ISRs are defined in Assembly */
extern void isr0();
extern void isr1();
extern void isr2();
extern void isr3();
extern void isr4();
extern void isr5();
extern void isr6();
extern void isr7();
extern void isr8();
extern void isr9();
extern void isr10();
extern void isr11();
extern void isr12();
extern void isr13();
extern void isr14();
extern void isr15();
extern void isr16();
extern void isr17();
extern void isr18();
extern void isr19();
extern void isr20();
extern void isr21();
extern void isr22();
extern void isr23();
extern void isr24();
extern void isr25();
extern void isr26();
extern void isr27();
extern void isr28();
extern void isr29();
extern void isr30();
extern void isr31();

/**
 * Install the Interrupt Service Routines (ISRs) into the IDT.
 */
void isr_install(void);

void isr_handler(struct cpu_registers *regs);
void irq_handler(struct cpu_registers *regs);

#endif /* IDT_H */