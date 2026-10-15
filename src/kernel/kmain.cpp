#include <cstdint>

// Limine requests go here...

extern "C" void kmain(void) {
    // Welcome to Ring 0 C++!
    while (true) {
        __asm__ volatile("hlt");
    }
}