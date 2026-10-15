#pragma once

#include <cstdint>

namespace cpu {

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

    uint64_t tss_low;  /* low 64 bits of the TSS base address */
    uint64_t tss_high; /* high 64 bits of the TSS base address */
};

/**
 * 64-bit Task State Segment structure.
 */
struct alignas(16) TaskStateSegment {
    uint32_t reserved0;
    uint64_t rsp0; /* kernel stack pointer */
    uint64_t rsp1; /* ring 1 stack pointer? */
    uint64_t rsp2; /* ring 2 stack pointer? */
    uint64_t reserved1;
    uint64_t ist[7]; /* interrupt stack table pointers */
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t io_map_base;
} __attribute__((packed));

[[maybe_unused]] static GDT gdt;
[[maybe_unused]] static GDTPointer gdt_ptr;
[[maybe_unused]] static TaskStateSegment kernel_tss;

void initialize_gdt();

/**
 * Load the Global Descriptor Table (GDT) and update segment registers.
 */
extern "C" void load_gdt(const GDTPointer* gdt_ptr, uint16_t cs, uint16_t ds);

/**
 * Load the Task State Segment (TSS) with the given segment selector.
 */
extern "C" void load_tss(uint16_t tss_selector);

}  // namespace cpu
