#ifndef CELESTEOS_GDT_H
#define CELESTEOS_GDT_H

#include <cstdint>

/**
 * An entry in the Global Descriptor Table (GDT).
 */
struct alignas(8) GDTEntry {
    uint16_t limit_low;
    uint16_t base_low;   /* low 16 bits of the base address */
    uint8_t base_middle; /* middle 8 bits of the base address */
    uint8_t access;
    uint8_t granularity;
    uint8_t base_high; /* high 8 bits of the base address */

    /**
     * Returns a null GDT entry.
     */
    static constexpr GDTEntry null_entry() {
        return GDTEntry{0, 0, 0, 0, 0, 0};
    }

    static constexpr GDTEntry create(uint8_t access_flags, uint8_t granularity_flags) {
        return GDTEntry{
            .limit_low = 0,
            .base_low = 0,
            .base_middle = 0,
            .access = access_flags,
            .granularity = granularity_flags,
            .base_high = 0,
        };
    }
};

/**
 * Pointer to the Global Descriptor Table (GDT)
 */
struct alignas(8) GDTPointer {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

/**
 * The Global Descriptor Table (GDT) containing all segment descriptors.
 */
struct GDT {
    GDTEntry null_descriptor; /* null descriptor: offset 0x00 */
    GDTEntry kernel_code;     /* kernel code segment: 0x08 (access: 0x9A, flags: 0x20) */
    GDTEntry kernel_data;     /* kernel data segment: 0x10 (access: 0x92, flags: 0x00) */
    GDTEntry user_data;       /* user data segment: 0x18 (access: 0xF2, flags: 0x00) */
    GDTEntry user_code;       /* user code segment: 0x20 (access: 0xFA, flags: 0x20) */
};

[[maybe_unused]] static GDT gdt;
[[maybe_unused]] static GDTPointer gdt_ptr;

/**
 * Load the Global Descriptor Table (GDT) and update segment registers.
 */
extern "C" void load_gdt(const GDTPointer* gdt_ptr, uint16_t cs, uint16_t ds);

void initialize_gdt();

#endif  // CELESTEOS_GDT_H