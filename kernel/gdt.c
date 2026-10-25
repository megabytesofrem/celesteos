#include "gdt.h"

#include <kernel/tss.h>

/* Kernel Global Descriptor Table (GDT) */
static gdt_t k_gdt;
static gdt_pointer_t k_gdt_ptr;

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

void gdt_init(void) {
    /* 0x00: Null descriptor */
    k_gdt.null_descriptor = gdt_entry_null();

    /* 0x08: Kernel code */
    k_gdt.kernel_code = gdt_entry_create(GDT_KERNEL_CODE, GDT_GRANULARITY_LONG);

    /* 0x10: Kernel data */
    k_gdt.kernel_data = gdt_entry_create(GDT_KERNEL_DATA, GDT_GRANULARITY_LONG);

    /* 0x18: User code */
    k_gdt.user_code = gdt_entry_create(GDT_USER_CODE, GDT_GRANULARITY_LONG);

    /* 0x20: User data */
    k_gdt.user_data = gdt_entry_create(GDT_USER_DATA, GDT_GRANULARITY_LONG);

    tss_init();
    uintptr_t tss_base = (uintptr_t)tss_get();
    uint64_t tss_limit = sizeof(tss_t) - 1;
    /* Available 64-bit TSS, present at ring 0, with byte granularity. */
    k_gdt.tss_low = (tss_limit & 0xFFFF) | ((tss_base & 0xFFFFFF) << 16) | (UINT64_C(0x89) << 40) |
                    (((tss_limit >> 16) & 0xF) << 48) | (((tss_base >> 24) & 0xFF) << 56);
    k_gdt.tss_high = tss_base >> 32;

    k_gdt_ptr.limit = sizeof(k_gdt) - 1;
    k_gdt_ptr.base = (uint64_t)&k_gdt;

    load_gdt(&k_gdt_ptr, GDT_KERNEL_CS, GDT_KERNEL_DS);
    load_tss(GDT_TSS);

    return;
}