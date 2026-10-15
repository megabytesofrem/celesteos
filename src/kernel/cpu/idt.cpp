#include "idt.h"

/**
 * The Interrupt Descriptor Table (IDT).
 */
IDTEntry idt[256];

IDTPointer idt_ptr;

/**
 * Set the IDT entry for a specific interrupt vector.
 */
void idt_set_entry(int entry, uint64_t handler, int type_attr) {
    // Create the entry in the IDT
    IDTEntry entry_struct = {
        .offset_low = static_cast<uint16_t>(handler & 0xFFFF),
        .selector = 0x08, /* kernel code segment in GDT */
        .ist = 0x0,       /* Interrupt Stack Table index, zero = default stack */
        .type_attr = static_cast<uint8_t>(type_attr),
        .offset_middle = static_cast<uint16_t>((handler >> 16) & 0xFFFF),
        .offset_high = static_cast<uint32_t>((handler >> 32) & 0xFFFFFFFF),
        .zero = 0x0, /* reserved, set to zero */
    };

    // Set the entry in the IDT
    idt[entry] = entry_struct;
}

/**
 * Initialize the IDT and load it onto the CPU.
 */
void initialize_idt() {
    // Initialize all IDT entries to zero
    for (int i = 0; i < 256; ++i) {
        idt[i] = {};
    }

    // Set up the IDT pointer
    idt_ptr.limit = sizeof(idt) - 1;
    idt_ptr.base = reinterpret_cast<uint64_t>(&idt);

    // Load the IDT
    asm volatile("lidt %0" : : "m"(idt_ptr));
}