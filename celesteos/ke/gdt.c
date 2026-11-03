#include "gdt.h"

#include <ke/tss.h>

/* Kernel Global Descriptor Table (GDT) */
static KeGDT g_gdt;
static KeGDTPointer g_gdt_ptr;

KeGDTEntry KeNullGDTEntry() {
    KeGDTEntry entry = {0};
    return entry;
}

KeGDTEntry KeCreateGDTEntry(uint8_t access, uint8_t granularity) {
    KeGDTEntry entry = {
        .limit_low = 0,
        .base_low = 0,
        .base_middle = 0,
        .access = access,
        .granularity = granularity,
        .base_high = 0,
    };
    return entry;
}

void KeInitGDT(void) {
    /* 0x00: Null descriptor */
    g_gdt.null_descriptor = KeNullGDTEntry();

    /* 0x08: Kernel code */
    g_gdt.kernel_code = KeCreateGDTEntry(GDT_KERNEL_CODE, GDT_GRANULARITY_LONG);

    /* 0x10: Kernel data */
    g_gdt.kernel_data = KeCreateGDTEntry(GDT_KERNEL_DATA, GDT_GRANULARITY_LONG);

    /* 0x18: User code */
    g_gdt.user_code = KeCreateGDTEntry(GDT_USER_CODE, GDT_GRANULARITY_LONG);

    /* 0x20: User data */
    g_gdt.user_data = KeCreateGDTEntry(GDT_USER_DATA, GDT_GRANULARITY_LONG);

    KeTssInitialize();
    uintptr_t tss_base = (uintptr_t)KeTssGet();
    uint64_t tss_limit = sizeof(KeTaskStateSegment) - 1;
    /* Available 64-bit TSS, present at ring 0, with byte granularity. */
    g_gdt.tss_low = (tss_limit & 0xFFFF) | ((tss_base & 0xFFFFFF) << 16) | (UINT64_C(0x89) << 40) |
                    (((tss_limit >> 16) & 0xF) << 48) | (((tss_base >> 24) & 0xFF) << 56);
    g_gdt.tss_high = tss_base >> 32;

    g_gdt_ptr.limit = sizeof(g_gdt) - 1;
    g_gdt_ptr.base = (uint64_t)&g_gdt;

    KeLoadGDT(&g_gdt_ptr, GDT_KERNEL_CS, GDT_KERNEL_DS);
    KeLoadTSS(GDT_TSS);

    return;
}