#include "gdt.h"

/**
 * Initialize the GDT and load it into the CPU.
 */
void initialize_gdt() {
    // Null descriptor
    gdt.null_descriptor = GDTEntry::null_entry();

    // Kernel code segment (ring 0, executable, long mode)
    gdt.kernel_code = GDTEntry::create(0x9A, 0x20);

    // Kernel data segment (ring 0, writable)
    gdt.kernel_data = GDTEntry::create(0x92, 0x00);

    // User data segment (ring 3, writable)
    gdt.user_data = GDTEntry::create(0xF2, 0x00);

    // User code segment (ring 3, executable, long mode)
    gdt.user_code = GDTEntry::create(0xFA, 0x20);

    // Set up the GDT pointer
    gdt_ptr.limit = sizeof(GDT) - 1;
    gdt_ptr.base = reinterpret_cast<uint64_t>(&gdt);

    // Load GDT and reload segment registers
    load_gdt(&gdt_ptr, 0x08, 0x10);
}