#include "idt.h"

/**
 * The Interrupt Descriptor Table (IDT).
 */
idt_entry_t idt[256];

idt_pointer_t idt_ptr;

/**
 * Set the IDT entry to an assembly interrupt stub for a specific vector.
 */
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
    idt[entry] = entry_struct;
}

/**
 * Initialize the IDT and load it onto the CPU.
 */
void idt_initialize(void) {
    __asm__ volatile("cli" : : : "memory");
    // Initialize all IDT entries to zero
    for (int i = 0; i < 256; ++i) {
        idt[i] = (idt_entry_t){
            .offset_low = 0,
            .selector = 0,
            .ist = 0,
            .type_attr = 0,
            .offset_middle = 0,
            .offset_high = 0,
            .zero = 0,
        };
    }

    // Set up the IDT pointer
    idt_load();
}

void idt_load(void) {
    /* Fill the IDT descriptor */
    struct idt_pointer_t ptr;

    ptr.base = (uint64_t)idt;
    ptr.limit = sizeof(idt) - 1;

    /* Load the IDT */
    __asm__ volatile("lidtq %0" : : "m"(ptr) : "memory");
}