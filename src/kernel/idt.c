#include "idt.h"
#include "isr_stubs.h"

/**
 * The Interrupt Descriptor Table (IDT).
 */
static __attribute__((aligned(16))) idt_entry_t idt[256];

static idt_pointer_t idt_ptr;

void idt_initialize(void) {
    _Static_assert(sizeof(idt_entry_t) == 16, "idt_entry_t must be exactly 16 bytes!");
    _Static_assert(sizeof(idt) == 4096, "idt array must be exactly 4096 bytes!");

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

    // Set up the #DF handler (Double Fault) with IST index 1
    idt_set_entry_ist(8, isr8, 0x8E, 1);

    // Set up the #PF handler (Page Fault) with IST index 2
    idt_set_entry_ist(14, isr14, 0x8E, 2);

    // Set up the IDT pointer
    idt_load();
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
    idt[entry] = entry_struct;
}

void idt_set_entry_ist(int entry, idt_handler_t handler, int type_attr, int ist_index) {
    uintptr_t addr = (uintptr_t)handler;

    idt[entry] = (idt_entry_t){
        .offset_low = (uint16_t)(addr & 0xFFFF),
        .selector = 0x08,        /* Kernel Code Segment */
        .ist = ist_index & 0x07, /* Bits 0..2 hold IST index (1-7) */
        .type_attr = type_attr,  /* e.g., 0x8E for Present, Ring 0 */
        .offset_middle = (uint16_t)((addr >> 16) & 0xFFFF),
        .offset_high = (uint32_t)((addr >> 32) & 0xFFFFFFFF),
        .zero = 0x0
    };
}

void idt_load(void) {
    /* Fill the IDT descriptor */
    struct idt_pointer_t ptr;

    ptr.base = (uint64_t)idt;
    ptr.limit = sizeof(idt) - 1;

    /* Load the IDT */
    __asm__ volatile("lidtq %0" : : "m"(ptr) : "memory");
}