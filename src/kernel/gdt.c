#include "gdt.h"
#include "task.h"

#include <libkernel.h>
#include <string.h>

static gdt_t gdt;
static gdt_pointer_t gdt_ptr;

gdt_entry_t gdt_entry_null() {
    gdt_entry_t entry = { 0 };
    return entry;
}

gdt_entry_t gdt_entry_create(uint8_t access, uint8_t granularity) {
    gdt_entry_t entry = { .limit_low = 0,
                          .base_low = 0,
                          .base_middle = 0,
                          .access = access,
                          .granularity = granularity,
                          .base_high = 0 };
    return entry;
}

void gdt_initialize() {

    /* 0x00: Null descriptor */
    gdt.null_descriptor = gdt_entry_null();

    /* 0x08: Kernel code segment */
    gdt.kernel_code = gdt_entry_create(GDT_KERNEL_CODE, GDT_GRANULARITY_LONG);

    /* 0x10: Kernel data segment (Ring 0, Writable) */
    gdt.kernel_data = gdt_entry_create(GDT_KERNEL_DATA, 0x00);

    /* 0x18: User code segment (Ring 3, Executable, Long mode) */
    gdt.user_code = gdt_entry_create(GDT_USER_CODE, GDT_GRANULARITY_LONG);

    /* 0x20: User data segment (Ring 3, Writable) */
    gdt.user_data = gdt_entry_create(GDT_USER_DATA, 0x00);

    // Setup the Task State Segment (TSS)
    tss_initialize();

    uint64_t tss_base = (uint64_t)&kernel_tss;
    uint64_t tss_limit = sizeof(tss_t) - 1;

    /* 0x28: TSS Descriptor (Occupies 16 bytes across 0x28 and 0x30) */
    gdt.tss_low = (tss_limit & 0xFFFF) | ((tss_base & 0xFFFF) << 16) |
                  (((tss_base >> 16) & 0xFF) << 32) | (0x89ULL << 40) |
                  (((tss_limit >> 16) & 0x0F) << 48) | (((tss_base >> 24) & 0xFF) << 56);

    /* High 8 bytes of the TSS descriptor */
    gdt.tss_high = tss_base >> 32;

    // Set up the GDT pointer
    gdt_ptr.limit = sizeof(gdt) - 1;
    gdt_ptr.base = (uint64_t)(&gdt);

    // Load GDT and reload segment registers
    load_gdt(&gdt_ptr, GDT_KERNEL_CS, GDT_KERNEL_DS);

    // Load TSS
    load_tss(GDT_TSS); // TSS segment selector (offset 0x28 in GDT)

    return;
}
