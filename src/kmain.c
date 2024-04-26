#include "multiboot.h"
#include <device/vga_text.h>

int kmain(multiboot_info_t *info) {
    vga_init();
    vga_print("Hello, World!", 13);

    // for (;;) {
    //     asm("hlt");
    // }
}