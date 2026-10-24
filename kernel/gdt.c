#include "gdt.h"

/* Global Descriptor Table (GDT) */
static gdt_t GDT;
static gdt_pointer_t GDT_PTR;

gdt_entry_t gdt_entry_null() {
    gdt_entry_t entry = { 0 };
    return entry;
}

gdt_entry_t gdt_entry_create(uint8_t access, uint8_t granularity) {
    gdt_entry_t entry = {
        .limit_low = 0,
        .base_low = 0,
        .base_middle = 0,
        .access = access,
        .granularity = granularity,
        .base_high = 0,
    };
    return entry;
}

void gdt_init() {
    /* 0x00: Null descriptor */
    GDT.null_descriptor = gdt_entry_null();

    /* 0x08: Kernel code */
    GDT.kernel_code = gdt_entry_create(GDT_KERNEL_CODE, GDT_GRANULARITY_LONG);

    /* 0x10: Kernel data */
    GDT.kernel_data = gdt_entry_create(GDT_KERNEL_DATA, GDT_GRANULARITY_LONG);

    /* 0x18: User code */
    GDT.user_code = gdt_entry_create(GDT_USER_CODE, GDT_GRANULARITY_LONG);

    /* 0x20: User data */
    GDT.user_data = gdt_entry_create(GDT_USER_DATA, GDT_GRANULARITY_LONG);

    /* Set Task State Segment (TSS) */
    /* TODO: Needs page mapping first */

    GDT_PTR.limit = sizeof(GDT) - 1;
    GDT_PTR.base = (uint64_t)&GDT;

    load_gdt(&GDT_PTR, GDT_KERNEL_CS, GDT_KERNEL_DS);

    return;
}