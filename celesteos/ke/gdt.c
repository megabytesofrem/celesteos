#include "gdt.h"

#include <ke/tss.h>

/* Kernel Global Descriptor Table (GDT) */
static KeGDT gGdt;
static KeGDTPointer gGdtPtr;

KeGDTEntry KeNullGDTEntry() {
    KeGDTEntry entry = {0};
    return entry;
}

KeGDTEntry KeCreateGDTEntry(uint8_t access, uint8_t granularity) {
    KeGDTEntry entry = {
        .limitLow = 0,
        .baseLow = 0,
        .baseMiddle = 0,
        .access = access,
        .granularity = granularity,
        .baseHigh = 0,
    };
    return entry;
}

void KeInitGDT(void) {
    /* 0x00: Null descriptor */
    gGdt.nullDescriptor = KeNullGDTEntry();

    /* 0x08: Kernel code */
    gGdt.kernelCode = KeCreateGDTEntry(GDT_KERNEL_CODE, GDT_GRANULARITY_LONG);

    /* 0x10: Kernel data */
    gGdt.kernelData = KeCreateGDTEntry(GDT_KERNEL_DATA, GDT_GRANULARITY_LONG);

    /* 0x18: User code */
    gGdt.userCode = KeCreateGDTEntry(GDT_USER_CODE, GDT_GRANULARITY_LONG);

    /* 0x20: User data */
    gGdt.userData = KeCreateGDTEntry(GDT_USER_DATA, GDT_GRANULARITY_LONG);

    KeTssInitialize();
    uintptr_t tssBase = (uintptr_t)KeTssGet();
    uint64_t tssLimit = sizeof(KeTaskStateSegment) - 1;
    /* Available 64-bit TSS, present at ring 0, with byte granularity. */
    gGdt.tssLow = (tssLimit & 0xFFFF) | ((tssBase & 0xFFFFFF) << 16) | (UINT64_C(0x89) << 40) |
                  (((tssLimit >> 16) & 0xF) << 48) | (((tssBase >> 24) & 0xFF) << 56);
    gGdt.tssHigh = tssBase >> 32;

    gGdtPtr.limit = sizeof(gGdt) - 1;
    gGdtPtr.base = (uint64_t)&gGdt;

    KeLoadGDT(&gGdtPtr, GDT_KERNEL_CS, GDT_KERNEL_DS);
    KeLoadTSS(GDT_TSS);

    return;
}