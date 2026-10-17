#include "gdt.h"
#include "task.h"

#include <string.h>

static gdt_t gdt;
static gdt_pointer_t gdt_ptr;

gdt_entry_t gdt_entry_null() {
    gdt_entry_t entry = {0};
    return entry;
}

gdt_entry_t gdt_entry_create(uint8_t access, uint8_t granularity) {
    gdt_entry_t entry = {
        .limit_low = 0,
        .base_low = 0,
        .base_middle = 0,
        .access = access,
        .granularity = granularity,
        .base_high = 0
    };
    return entry;
}

void gdt_initialize() {
    gdt.null_descriptor = gdt_entry_null(); /* null descriptor */
    gdt.kernel_code =
        gdt_entry_create(0x9A, 0x20); /* kernel code segment, ring 0, executable, long mode */
    gdt.kernel_data = gdt_entry_create(0x92, 0x00); /* kernel data segment, ring 0, writable */
    gdt.user_data = gdt_entry_create(0xF2, 0x00);   /* user data segment, ring 3, writable */
    gdt.user_code =
        gdt_entry_create(0xFA, 0x20); /* user code segment, ring 3, executable, long mode */

    // Setup the Task State Segment (TSS)
    tss_initialize();

    uint64_t tss_base = (uint64_t)&kernel_tss;
    uint64_t tss_limit = sizeof(tss_t) - 1;

    // Encode 16-byte TSS into GDT
    gdt.tss_low = (tss_limit & 0xFFFF) | ((tss_base & 0xFFFF) << 16) |
                  (((tss_base >> 16) & 0xFF) << 32) |
                  (0x89ULL << 40) | // Present, Ring 0, Type 0x9 (Available 64-bit TSS)
                  (((tss_limit >> 16) & 0x0F) << 48) | (((tss_base >> 24) & 0xFF) << 56);

    gdt.tss_high = tss_base >> 32;

    // Set up the GDT pointer
    gdt_ptr.limit = sizeof(gdt) - 1;
    gdt_ptr.base = (uint64_t)(&gdt);

    // Load GDT and reload segment registers
    load_gdt(&gdt_ptr, 0x08, 0x10);

    // Load TSS
    load_tss(0x28); // TSS segment selector (offset 0x28 in GDT)

    return;
}
