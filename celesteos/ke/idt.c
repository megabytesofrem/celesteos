#include "idt.h"

#include <ke/pic.h>
#include <ke/tss.h>
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
static __attribute__((aligned(16))) KeIDTEntry g_idt[256];

static KeIDTPointer g_idt_ptr;

void KeInitIDT(void) {
    _Static_assert(sizeof(KeIDTEntry) == 16, "idt_entry_t must be exactly 16 bytes!");
    _Static_assert(sizeof(g_idt) == 4096, "idt array must be exactly 4096 bytes!");

    // Clear all interrupts
    __asm__ volatile("cli" : : : "memory");

    // Initialize all IDT entries to zero
    for (int i = 0; i < 256; ++i) {
        g_idt[i] = (KeIDTEntry){
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
    KeIDTSetEntryIST(8, isr8, 0x8E, KE_TSS_IST_DOUBLE_FAULT);

    // Set up the #PF handler (Page Fault) with IST index 2
    KeIDTSetEntryIST(14, isr14, 0x8E, KE_TSS_IST_PAGE_FAULT);

    // Load the IDT
    KeLoadIDT(&g_idt_ptr);
}

void KeIDTSetEntry(int entry, KeIDTHandler handler, int type_attr) {
    uintptr_t handler_address = (uintptr_t)handler;

    // Create the entry in the IDT
    KeIDTEntry entry_struct = {
        .offset_low = (uint16_t)(handler_address & 0xFFFF),
        .selector = 0x08, /* kernel code segment in GDT */
        .ist = 0x0,       /* Interrupt Stack Table index, zero = default stack */
        .type_attr = (uint8_t)(type_attr),
        .offset_middle = (uint16_t)((handler_address >> 16) & 0xFFFF),
        .offset_high = (uint32_t)((handler_address >> 32) & 0xFFFFFFFF),
        .zero = 0x0, /* reserved, set to zero */
    };

    // Set the entry in the IDT
    g_idt[entry] = entry_struct;
}

void KeIDTSetEntryIST(int entry, KeIDTHandler handler, int type_attr, int ist_index) {
    uintptr_t addr = (uintptr_t)handler;

    g_idt[entry] = (KeIDTEntry){
        .offset_low = (uint16_t)(addr & 0xFFFF),
        .selector = 0x08,        /* Kernel Code Segment */
        .ist = ist_index & 0x07, /* Bits 0..2 hold IST index (1-7) */
        .type_attr = type_attr,  /* e.g., 0x8E for Present, Ring 0 */
        .offset_middle = (uint16_t)((addr >> 16) & 0xFFFF),
        .offset_high = (uint32_t)((addr >> 32) & 0xFFFFFFFF),
        .zero = 0x0
    };
}

void KeLoadIDT(KeIDTPointer *ptr) {
    ptr->base = (uint64_t)g_idt;
    ptr->limit = sizeof(g_idt) - 1;

    // Load the IDT
    __asm__ volatile("lidtq %0" : : "m"(*ptr) : "memory");
}

/* ISR */

void KeInstallISRHandlers(void) {
    __asm__ volatile("cli" : : : "memory");
    KeIDTSetEntry(0, isr0, 0x8f);
    KeIDTSetEntry(1, isr1, 0x8f);
    KeIDTSetEntry(2, isr2, 0x8f);
    KeIDTSetEntry(3, isr3, 0x8f);
    KeIDTSetEntry(4, isr4, 0x8f);
    KeIDTSetEntry(5, isr5, 0x8f);
    KeIDTSetEntry(6, isr6, 0x8f);
    KeIDTSetEntry(7, isr7, 0x8f);
    KeIDTSetEntryIST(8, isr8, 0x8e, KE_TSS_IST_DOUBLE_FAULT);
    KeIDTSetEntry(9, isr9, 0x8f);
    KeIDTSetEntry(10, isr10, 0x8f);
    KeIDTSetEntry(11, isr11, 0x8f);
    KeIDTSetEntry(12, isr12, 0x8f);
    KeIDTSetEntry(13, isr13, 0x8f);
    KeIDTSetEntryIST(14, isr14, 0x8e, KE_TSS_IST_PAGE_FAULT);
    KeIDTSetEntry(15, isr15, 0x8f);
    KeIDTSetEntry(16, isr16, 0x8f);
    KeIDTSetEntry(17, isr17, 0x8f);
    KeIDTSetEntry(18, isr18, 0x8f);
    KeIDTSetEntry(19, isr19, 0x8f);
    KeIDTSetEntry(20, isr20, 0x8f);
    KeIDTSetEntry(21, isr21, 0x8f);
    KeIDTSetEntry(22, isr22, 0x8f);
    KeIDTSetEntry(23, isr23, 0x8f);
    KeIDTSetEntry(24, isr24, 0x8f);
    KeIDTSetEntry(25, isr25, 0x8f);
    KeIDTSetEntry(26, isr26, 0x8f);
    KeIDTSetEntry(27, isr27, 0x8f);
    KeIDTSetEntry(28, isr28, 0x8f);
    KeIDTSetEntry(29, isr29, 0x8f);
    KeIDTSetEntry(30, isr30, 0x8f);
    KeIDTSetEntry(31, isr31, 0x8f);

    /* IRQs */
    KeIDTSetEntry(32, irq0, 0x8e); /* 0x8e = INTERRUPT_GATE */
    KeIDTSetEntry(33, irq1, 0x8e);
    KeIDTSetEntry(34, irq2, 0x8e);
    KeIDTSetEntry(35, irq3, 0x8e);
    KeIDTSetEntry(36, irq4, 0x8e);
    KeIDTSetEntry(37, irq5, 0x8e);
    KeIDTSetEntry(38, irq6, 0x8e);
    KeIDTSetEntry(39, irq7, 0x8e);
    KeIDTSetEntry(40, irq8, 0x8e);
    KeIDTSetEntry(41, irq9, 0x8e);
    KeIDTSetEntry(42, irq10, 0x8e);
    KeIDTSetEntry(43, irq11, 0x8e);
    KeIDTSetEntry(44, irq12, 0x8e);
    KeIDTSetEntry(45, irq13, 0x8e);
    KeIDTSetEntry(46, irq14, 0x8e);
    KeIDTSetEntry(47, irq15, 0x8e);

    // Remap both PICs before allowing interrupts on vectors 32-47.
    KeRemapPIC(0x20, 0x28);
    KeLoadIDT(&g_idt_ptr);
    __asm__ volatile("sti" : : : "memory");

    return;
}

void KeIRQHandler(struct KeCPUFrame *regs) {
    KePICSendEOI((uint8_t)(regs->int_no - 32));
}

void KeISRHandler(struct KeCPUFrame *regs) {
    const char *message = regs->int_no < sizeof(exception_messages) / sizeof(exception_messages[0])
                              ? exception_messages[regs->int_no]
                              : "Unknown Exception";

    // Print the exception message
    (void)message;

    for (;;) {
        __asm__ volatile("cli; hlt" : : : "memory");
    }
}