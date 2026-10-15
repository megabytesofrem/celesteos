#include "gdt.h"

namespace cpu {

/**
 * Initialize the GDT and load it into the CPU.
 */
void initialize_gdt() {
    gdt.null_descriptor = GDTEntry::null_entry();   /* null descriptor */
    gdt.kernel_code = GDTEntry::create(0x9A, 0x20); /* kernel code segment, ring 0, executable, long mode */
    gdt.kernel_data = GDTEntry::create(0x92, 0x00); /* kernel data segment, ring 0, writable */
    gdt.user_data = GDTEntry::create(0xF2, 0x00);   /* user data segment, ring 3, writable */
    gdt.user_code = GDTEntry::create(0xFA, 0x20);   /* user code segment, ring 3, executable, long mode */

    // Setup the Task State Segment (TSS)
    uint64_t tss_base = reinterpret_cast<uint64_t>(&kernel_tss);
    uint64_t tss_limit = sizeof(TaskStateSegment) - 1;

    // Encode 16-byte TSS into GDT
    gdt.tss_low = (tss_limit & 0xFFFF) |
                  ((tss_base & 0xFFFF) << 16) |
                  ((tss_base & 0xFF0000) << 16) |
                  (0x89ULL << 40) |  // Present, Type 0x9 (64-bit TSS available)
                  (((tss_limit >> 16) & 0x0F) << 48) |
                  (((tss_base >> 24) & 0xFF) << 56);
    gdt.tss_high = tss_base >> 32;

    // Set up the GDT pointer
    gdt_ptr.limit = sizeof(GDT) - 1;
    gdt_ptr.base = reinterpret_cast<uint64_t>(&gdt);

    // Load GDT and reload segment registers
    load_gdt(&gdt_ptr, 0x08, 0x10);

    // Load TSS
    load_tss(0x28);  // TSS segment selector (offset 0x28 in GDT)
}

}  // namespace cpu