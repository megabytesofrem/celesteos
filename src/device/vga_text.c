/**
 * vga_text.c: VGA 80x25 text-mode driver
 */
#include "vga_text.h"

size_t vga_row, vga_col;
u8 vga_color;
u16 *vga_buffer;

void vga_init() {
    vga_row = 0;
    vga_col = 0;
    vga_color = vga_entrycolor(VGA_LIGHT_GREY, VGA_BLACK);
    vga_buffer = (u16 *)0xB8000;
    vga_clear();
}

void vga_setcolor(u8 color) { vga_color = color; }

void vga_put(char c, u8 color, size_t x, size_t y) {
    vga_buffer[y * VGA_TEXTMODE_WIDTH + x] = (u16)c | (u16)color << 8;
}

void vga_putc(char c) {
    if (c == '\n') {
        vga_col = 0;
        vga_row++;
        return;
    }

    // Reset column if it exceeds the width
    if (++vga_col == VGA_TEXTMODE_WIDTH) {
        vga_col = 0;
        if (++vga_row == VGA_TEXTMODE_HEIGHT)
            vga_row = 0;
    }

    vga_put(c, vga_color, vga_col, vga_row);
}

void vga_print(const char *s, size_t len) {
    for (size_t i = 0; i < len; i++)
        vga_putc(s[i]);
}

void vga_clear() { memset(vga_buffer, 0, 80 * 25 * 2); }