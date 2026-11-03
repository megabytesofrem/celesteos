#ifndef CELESTEOS_GDT_H
#define CELESTEOS_GDT_H

#include <stddef.h>
#include <stdint.h>

#include <stddef.h>
#include <stdint.h>

/**
 * GDT access byte flags.
 */
#define GDT_ACCESS_PRESENT (1 << 7)    /* bit 7: segment present */
#define GDT_ACCESS_RING0 (0 << 5)      /* bit 6-5: descriptor privilege level 0 */
#define GDT_ACCESS_RING3 (3 << 5)      /* bit 6-5: descriptor privilege level 3 */
#define GDT_ACCESS_DESCRIPTOR (1 << 4) /* bit 4: descriptor type (0 = system, 1 = code/data) */
#define GDT_ACCESS_EXECUTABLE (1 << 3) /* bit 3: executable segment */
#define GDT_ACCESS_DIRECTION (1 << 2)  /* bit 2: direction/conforming */
#define GDT_ACCESS_RW (1 << 1)         /* bit 1: readable/writable */
#define GDT_ACCESS_ACCESSED (1 << 0)   /* bit 0: accessed */

/**
 * GDT granularity byte flags.
 */
#define GDT_GRANULARITY_4K (1 << 7)    /* bit 7: granularity (0 = 1B-1MB, 1 = 4KB-4GB) */
#define GDT_GRANULARITY_32BIT (1 << 6) /* bit 6: 32 bit protected mode */
#define GDT_GRANULARITY_LONG (1 << 5)  /* bit 5: long mode*/

/**
 * GDT convience macros.
 */

/* Kernel code: Ring 0, executable, read + write */
#define GDT_OFFSET_NULL 0x00
#define GDT_OFFSET_KERNEL_CODE 0x08
#define GDT_OFFSET_KERNEL_DATA 0x10
#define GDT_OFFSET_USER_CODE 0x18
#define GDT_OFFSET_USER_DATA 0x20
#define GDT_OFFSET_TSS 0x28

/* Descriptors */
#define GDT_KERNEL_CODE                                                                            \
    (GDT_ACCESS_PRESENT | GDT_ACCESS_RING0 | GDT_ACCESS_DESCRIPTOR | GDT_ACCESS_EXECUTABLE |       \
        GDT_ACCESS_RW)
#define GDT_KERNEL_DATA                                                                            \
    (GDT_ACCESS_PRESENT | GDT_ACCESS_RING0 | GDT_ACCESS_DESCRIPTOR | GDT_ACCESS_RW)
#define GDT_USER_CODE                                                                              \
    (GDT_ACCESS_PRESENT | GDT_ACCESS_RING3 | GDT_ACCESS_DESCRIPTOR | GDT_ACCESS_EXECUTABLE |       \
        GDT_ACCESS_RW)
#define GDT_USER_DATA                                                                              \
    (GDT_ACCESS_PRESENT | GDT_ACCESS_RING3 | GDT_ACCESS_DESCRIPTOR | GDT_ACCESS_RW)

/* Segment selectors */
#define GDT_KERNEL_CS (GDT_OFFSET_KERNEL_CODE | 0)
#define GDT_KERNEL_DS (GDT_OFFSET_KERNEL_DATA | 0)
#define GDT_USER_CS (GDT_OFFSET_USER_CODE | 3)
#define GDT_USER_DS (GDT_OFFSET_USER_DATA | 3)
#define GDT_TSS (GDT_OFFSET_TSS | 0)

/**
 * An entry in the Global Descriptor Table (GDT).
 */
typedef struct __attribute__((packed)) KeGDTEntry {
    uint16_t limitLow;
    uint16_t baseLow;   /* low 16 bits of the base address */
    uint8_t baseMiddle; /* middle 8 bits of the base address */
    uint8_t access;
    uint8_t granularity;
    uint8_t baseHigh; /* high 8 bits of the base address */
} KeGDTEntry;

typedef struct __attribute__((packed)) KeGDTPointer {
    uint16_t limit;
    uint64_t base;
} KeGDTPointer;

/**
 * The GDT containing all segment descriptors.
 */
typedef struct __attribute__((packed)) KeGDT {
    KeGDTEntry nullDescriptor; /* null descriptor: offset 0x00 */
    KeGDTEntry kernelCode;     /* kernel code segment: 0x08 (access: 0x9A, flags: 0x20) */
    KeGDTEntry kernelData;     /* kernel data segment: 0x10 (access: 0x92, flags: 0x00) */
    KeGDTEntry userCode;       /* user code segment: 0x18 (access: 0xFA, flags: 0x20) */
    KeGDTEntry userData;       /* user data segment: 0x20 (access: 0xF2, flags: 0x00) */

    uint64_t tssLow;  /* low 64 bits of the TSS base address */
    uint64_t tssHigh; /* high 64 bits of the TSS base address */
} KeGDT;

_Static_assert(sizeof(KeGDTEntry) == 8, "GDT entries must be 8 bytes");
_Static_assert(sizeof(KeGDTPointer) == 10, "GDTR operands must be 10 bytes");
_Static_assert(offsetof(KeGDTPointer, base) == 2, "GDTR base must follow the 16-bit limit");
_Static_assert(offsetof(KeGDT, tssLow) == 0x28, "TSS descriptor must match selector 0x28");

/**
 * Initialize the Global Descriptor Table (GDT).
 */
void KeInitGDT(void);

KeGDTEntry KeNullGDTEntry();
KeGDTEntry KeCreateGDTEntry(uint8_t access, uint8_t granularity);

/**
 * Load the Global Descriptor Table (GDT) and update segment registers.
 */
extern void KeLoadGDT(const KeGDTPointer *gdtPtr, uint16_t cs, uint16_t ds);

/**
 * Load the Task State Segment (TSS) with the given segment selector.
 */
extern void KeLoadTSS(uint16_t tssSelector);

#endif /* CELESTEOS_GDT_H */