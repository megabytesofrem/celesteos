#ifndef CELESTEOS_GDT_H
#define CELESTEOS_GDT_H

#include <mach/kern_return.h>
#include <stddef.h>
#include <stdint.h>

/**
 * An entry in the Global Descriptor Table (GDT).
 */
typedef struct __attribute__((packed)) gdt_entry_t {
    uint16_t limit_low;
    uint16_t base_low;   /* low 16 bits of the base address */
    uint8_t base_middle; /* middle 8 bits of the base address */
    uint8_t access;
    uint8_t granularity;
    uint8_t base_high; /* high 8 bits of the base address */
} gdt_entry_t;

/**
 * Packed 10-byte LGDT operand: 16-bit limit followed by a 64-bit base.
 */
typedef struct __attribute__((packed)) gdt_pointer_t {
    uint16_t limit;
    uint64_t base;
} gdt_pointer_t;

/**
 * The Global Descriptor Table (GDT) containing all segment descriptors.
 */
typedef struct gdt_t {
    gdt_entry_t null_descriptor; /* null descriptor: offset 0x00 */
    gdt_entry_t kernel_code;     /* kernel code segment: 0x08 (access: 0x9A, flags: 0x20) */
    gdt_entry_t kernel_data;     /* kernel data segment: 0x10 (access: 0x92, flags: 0x00) */
    gdt_entry_t user_data;       /* user data segment: 0x18 (access: 0xF2, flags: 0x00) */
    gdt_entry_t user_code;       /* user code segment: 0x20 (access: 0xFA, flags: 0x20) */

    uint64_t tss_low;  /* low 64 bits of the TSS base address */
    uint64_t tss_high; /* high 64 bits of the TSS base address */
} gdt_t;

_Static_assert(sizeof(gdt_entry_t) == 8, "GDT entries must be 8 bytes");
_Static_assert(sizeof(gdt_pointer_t) == 10, "GDTR operands must be 10 bytes");
_Static_assert(offsetof(gdt_pointer_t, base) == 2,
               "GDTR base must follow the 16-bit limit");
_Static_assert(offsetof(gdt_t, tss_low) == 0x28,
               "TSS descriptor must match selector 0x28");

/**
 * Initialize the Global Descriptor Table (GDT).
 * 
 * This function sets up the GDT with the necessary segment descriptors and loads it into the GDTR.
 */
kern_return_t gdt_initialize();

gdt_entry_t gdt_entry_null();
gdt_entry_t gdt_entry_create(uint8_t access, uint8_t granularity);

/**
 * Load the Global Descriptor Table (GDT) and update segment registers.
 */
extern void load_gdt(const gdt_pointer_t* gdt_ptr, uint16_t cs, uint16_t ds);

/**
 * Load the Task State Segment (TSS) with the given segment selector.
 */
extern void load_tss(uint16_t tss_selector);

#endif  // CELESTEOS_GDT_H
