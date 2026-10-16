#include "isr.h"
#include "idt.h"
#include "isr_stubs.h"
#include "pic.h"
#include "ports.h"

#include <libkernel.h>

/* Macro to print the value of a CPU register */
#ifndef PRINT_REGISTER
#define PRINT_REGISTER(name) \
    printk("    " #name "=0x%p\n", 0xFFFFFFFF, 0x0000, \
           (void*)(uintptr_t)regs->name)
#endif

const char* exception_messages[] = {
    "Division By Zero",
    "Debug",
    "Non Maskable Interrupt",
    "Breakpoint",
    "Into Detected Overflow",
    "Out of Bounds",
    "Invalid Opcode",
    "No Coprocessor",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Bad TSS",
    "Segment Not Present",
    "Stack Fault",
    "General Protection Fault",
    "Page Fault",
    "Unknown Interrupt",
    "Coprocessor Fault",
    "Alignment Check",
    "Machine Check",
    "SIMD Floating-Point Exception",
    "Virtualization Exception",
    "Control Protection Exception",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "VMM Communication Exception",
    "Security Exception",
    "Reserved"};

_Static_assert(sizeof(exception_messages) / sizeof(exception_messages[0]) == 32,
               "Exception messages must cover vectors 0-31");

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

void isr_install(void) {
    __asm__ volatile("cli" : : : "memory");
    idt_set_entry(0, isr0, 0x8f);
    idt_set_entry(1, isr1, 0x8f);
    idt_set_entry(2, isr2, 0x8f);
    idt_set_entry(3, isr3, 0x8f);
    idt_set_entry(4, isr4, 0x8f);
    idt_set_entry(5, isr5, 0x8f);
    idt_set_entry(6, isr6, 0x8f);
    idt_set_entry(7, isr7, 0x8f);
    idt_set_entry_ist(8, isr8, 0x8e, 1);
    idt_set_entry(9, isr9, 0x8f);
    idt_set_entry(10, isr10, 0x8f);
    idt_set_entry(11, isr11, 0x8f);
    idt_set_entry(12, isr12, 0x8f);
    idt_set_entry(13, isr13, 0x8f);
    idt_set_entry_ist(14, isr14, 0x8e, 2);
    idt_set_entry(15, isr15, 0x8f);
    idt_set_entry(16, isr16, 0x8f);
    idt_set_entry(17, isr17, 0x8f);
    idt_set_entry(18, isr18, 0x8f);
    idt_set_entry(19, isr19, 0x8f);
    idt_set_entry(20, isr20, 0x8f);
    idt_set_entry(21, isr21, 0x8f);
    idt_set_entry(22, isr22, 0x8f);
    idt_set_entry(23, isr23, 0x8f);
    idt_set_entry(24, isr24, 0x8f);
    idt_set_entry(25, isr25, 0x8f);
    idt_set_entry(26, isr26, 0x8f);
    idt_set_entry(27, isr27, 0x8f);
    idt_set_entry(28, isr28, 0x8f);
    idt_set_entry(29, isr29, 0x8f);
    idt_set_entry(30, isr30, 0x8f);
    idt_set_entry(31, isr31, 0x8f);

    /* IRQs */
    idt_set_entry(32, irq0, 0x8e); /* 0x8e = INTERRUPT_GATE */
    idt_set_entry(33, irq1, 0x8e);
    idt_set_entry(34, irq2, 0x8e);
    idt_set_entry(35, irq3, 0x8e);
    idt_set_entry(36, irq4, 0x8e);
    idt_set_entry(37, irq5, 0x8e);
    idt_set_entry(38, irq6, 0x8e);
    idt_set_entry(39, irq7, 0x8e);
    idt_set_entry(40, irq8, 0x8e);
    idt_set_entry(41, irq9, 0x8e);
    idt_set_entry(42, irq10, 0x8e);
    idt_set_entry(43, irq11, 0x8e);
    idt_set_entry(44, irq12, 0x8e);
    idt_set_entry(45, irq13, 0x8e);
    idt_set_entry(46, irq14, 0x8e);
    idt_set_entry(47, irq15, 0x8e);

    // Remap both PICs before allowing interrupts on vectors 32-47.
    pic_remap(0x20, 0x28);
    idt_load();
    __asm__ volatile("sti" : : : "memory");

    return;
}

void irq_handler(struct cpu_registers* regs) {
    pic_send_eoi((uint8_t)(regs->int_no - 32));
}

void isr_handler(struct cpu_registers* regs) {
    const char* message =
        regs->int_no < sizeof(exception_messages) / sizeof(exception_messages[0])
            ? exception_messages[regs->int_no]
            : "Unknown Exception";

    klog(KLOG_ERROR, "exception: %s (%u), err code: 0x%p\n",
         message, (unsigned int)regs->int_no,
         (void*)(uintptr_t)regs->err_code);

    printk("=*=*= EXCEPTION =*=*=\n", 0xFFFFFFFF, 0x0000);

    PRINT_REGISTER(rip);
    PRINT_REGISTER(rsp);
    PRINT_REGISTER(cr2);
    PRINT_REGISTER(rflags);
    PRINT_REGISTER(cs);
    PRINT_REGISTER(ss);
    PRINT_REGISTER(rax);
    PRINT_REGISTER(rbx);
    PRINT_REGISTER(rcx);
    PRINT_REGISTER(rdx);
    PRINT_REGISTER(rsi);
    PRINT_REGISTER(rdi);
    PRINT_REGISTER(rbp);
    PRINT_REGISTER(r8);
    PRINT_REGISTER(r9);
    PRINT_REGISTER(r10);
    PRINT_REGISTER(r11);
    PRINT_REGISTER(r12);
    PRINT_REGISTER(r13);
    PRINT_REGISTER(r14);
    PRINT_REGISTER(r15);

    printk("=*=*= HALT =*=*=\n", 0xFFFFFFFF, 0x0000);

    for (;;) {
        __asm__ volatile("cli; hlt" : : : "memory");
    }
}