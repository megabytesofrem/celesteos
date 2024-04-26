#ifndef VGA_TEXT_DEVICE_H
#define VGA_TEXT_DEVICE_H

#include <libc/common.h>
#include <libc/string.h>

#define VGA_TEXTMODE_WIDTH 80
#define VGA_TEXTMODE_HEIGHT 25

enum vga_color {
    VGA_BLACK = 0,
    VGA_BLUE = 1,
    VGA_GREEN = 2,
    VGA_CYAN = 3,
    VGA_RED = 4,
    VGA_MAGENTA = 5,
    VGA_BROWN = 6,
    VGA_LIGHT_GREY = 7,
    VGA_DARK_GREY = 8,
    VGA_LIGHT_BLUE = 9,
    VGA_LIGHT_GREEN = 10,
    VGA_LIGHT_CYAN = 11,
    VGA_LIGHT_RED = 12,
    VGA_LIGHT_MAGENTA = 13,
    VGA_LIGHT_BROWN = 14,
    VGA_WHITE = 15,
};

void vga_init();
void vga_setcolor(u8 color);
void vga_put(char c, u8 color, size_t x, size_t y);
void vga_putc(char c);
void vga_print(const char *s, size_t len);
void vga_clear();

static inline u16 vga_entrycolor(u8 fg, u8 bg) { return fg | bg << 4; }

#endif // VGA_TEXT_DEVICE_H