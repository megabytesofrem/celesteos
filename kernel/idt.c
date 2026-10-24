#include "idt.h"

#include <kernel/pic.h>
#include <stdint.h>

// clang-format off
const char *exception_messages[] = {
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
    "Reserved"
};
// clang-format on

/* Kernel Interrupt Descriptor Table (IDT) */
static __attribute__((aligned(16))) idt_entry_t k_idt[256];

static idt_pointer_t k_idt_ptr;

void idt_init(void) {
    _Static_assert(sizeof(idt_entry_t) == 16, "idt_entry_t must be exactly 16 bytes!");
    _Static_assert(sizeof(k_idt) == 4096, "idt array must be exactly 4096 bytes!");

    // Clear all interrupts
    __asm__ volatile("cli" : : : "memory");

    // Initialize all IDT entries to zero
    for (int i = 0; i < 256; ++i) {
        k_idt[i] = (idt_entry_t){
            .offset_low = 0,
            .selector = 0,
            .ist = 0,
            .type_attr = 0,
            .offset_middle = 0,
            .offset_high = 0,
            .zero = 0,
        };
    }

    // Set up the #DF handler (Double Fault) with IST index 1
    idt_set_entry_ist(8, isr8, 0x8E, 1);

    // Set up the #PF handler (Page Fault) with IST index 2
    idt_set_entry_ist(14, isr14, 0x8E, 2);

    // Load the IDT
    idt_load(&k_idt_ptr);
}

void idt_set_entry(int entry, idt_handler_t handler, int type_attr) {
    uintptr_t handler_address = (uintptr_t)handler;

    // Create the entry in the IDT
    idt_entry_t entry_struct = {
        .offset_low = (uint16_t)(handler_address & 0xFFFF),
        .selector = 0x08, /* kernel code segment in GDT */
        .ist = 0x0,       /* Interrupt Stack Table index, zero = default stack */
        .type_attr = (uint8_t)(type_attr),
        .offset_middle = (uint16_t)((handler_address >> 16) & 0xFFFF),
        .offset_high = (uint32_t)((handler_address >> 32) & 0xFFFFFFFF),
        .zero = 0x0, /* reserved, set to zero */
    };

    // Set the entry in the IDT
    k_idt[entry] = entry_struct;
}

void idt_set_entry_ist(int entry, idt_handler_t handler, int type_attr, int ist_index) {
    uintptr_t addr = (uintptr_t)handler;

    k_idt[entry] = (idt_entry_t){ .offset_low = (uint16_t)(addr & 0xFFFF),
        .selector = 0x08,        /* Kernel Code Segment */
        .ist = ist_index & 0x07, /* Bits 0..2 hold IST index (1-7) */
        .type_attr = type_attr,  /* e.g., 0x8E for Present, Ring 0 */
        .offset_middle = (uint16_t)((addr >> 16) & 0xFFFF),
        .offset_high = (uint32_t)((addr >> 32) & 0xFFFFFFFF),
        .zero = 0x0 };
}

void idt_load(struct idt_pointer_t *ptr) {
    ptr->base = (uint64_t)k_idt;
    ptr->limit = sizeof(k_idt) - 1;

    // Load the IDT
    __asm__ volatile("lidtq %0" : : "m"(*ptr) : "memory");
}

/* ISR */

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
    idt_load(&k_idt_ptr);
    __asm__ volatile("sti" : : : "memory");

    return;
}

void irq_handler(struct cpu_registers *regs) {
    pic_send_eoi((uint8_t)(regs->int_no - 32));
}

void isr_handler(struct cpu_registers *regs) {
    const char *message = regs->int_no < sizeof(exception_messages) / sizeof(exception_messages[0])
                              ? exception_messages[regs->int_no]
                              : "Unknown Exception";

    (void)message;

    /* TODO: implement logging */

    for (;;) {
        __asm__ volatile("cli; hlt" : : : "memory");
    }
}