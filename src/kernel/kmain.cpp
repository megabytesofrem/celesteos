#include <cstdint>

#include "cpu/gdt.h"
#include "cpu/idt.h"
#include "cpu/ports.h"

extern "C" void kmain(void) {
    // Initialize the Global Descriptor Table (GDT)
    initialize_gdt();

    // Initialize the Interrupt Descriptor Table (IDT)
    initialize_idt();

    // Write something to the Bochs/QEMU debug port (0xE9)
    e9_write("Hello, CelesteOS!\n");

    // Welcome to Ring 0 C++!
    while (true) {
        __asm__ volatile("hlt");
    }
}