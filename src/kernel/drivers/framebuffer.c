#include "framebuffer.h"

#include <stdlib/string.h>

#include "ter_u16b.h"

struct framebuffer_state global_framebuffer;

bool framebuffer_is_initialized(struct framebuffer_state* fb) {
    return (fb != NULL && fb->fb != NULL);
}

void framebuffer_init(struct framebuffer_state* fb, struct limine_framebuffer* limine_fb) {
    fb->fb = limine_fb;
    fb->pixel_data = (uint32_t*)limine_fb->address;
    fb->width = limine_fb->width;
    fb->height = limine_fb->height;
    fb->pitch_pixels = limine_fb->pitch / sizeof(uint32_t);
    fb->cursor_x = 0;
    fb->cursor_y = 0;

    framebuffer_clear(fb, 0x00000000);
}

void framebuffer_clear(struct framebuffer_state* fb, uint32_t bg_color) {
    for (size_t y = 0; y < fb->height; ++y) {
        for (size_t x = 0; x < fb->width; ++x) {
            framebuffer_draw_pixel(fb, x, y, bg_color);
        }
    }
}

void framebuffer_draw_pixel(struct framebuffer_state* fb, size_t x, size_t y, uint32_t color) {
    if (x >= fb->width || y >= fb->height) return;

    fb->pixel_data[y * fb->pitch_pixels + x] = color;
}

void framebuffer_draw_char(struct framebuffer_state* fb, char c, size_t x, size_t y, uint32_t fg_color, uint32_t bg_color) {
    uint8_t glyph_index = (uint8_t)c;

    // Get the glyph bitmap for the character from the font table
    const uint8_t* glyph = font_8x16[glyph_index];

    for (size_t row = 0; row < FONT_HEIGHT; row++) {
        uint8_t bitmap_row = glyph[row];

        for (size_t col = 0; col < FONT_WIDTH; col++) {
            bool is_set = (bitmap_row & (1 << (7 - col))) != 0;
            framebuffer_draw_pixel(fb, x + col, y + row, is_set ? fg_color : bg_color);
        }
    }
}

void framebuffer_putchar(struct framebuffer_state* fb, char c, uint32_t fg_color, uint32_t bg_color) {
    if (c == '\n') {
        fb->cursor_x = 0;
        fb->cursor_y += FONT_HEIGHT;
    } else if (c == '\r') {
        fb->cursor_x = 0;
    } else if (c == '\t') {
        fb->cursor_x = (fb->cursor_x + 4 * FONT_WIDTH) & ~(4 * FONT_WIDTH - 1);
    } else {
        framebuffer_draw_char(fb, c, fb->cursor_x, fb->cursor_y, fg_color, bg_color);
        fb->cursor_x += FONT_WIDTH;
    }

    // Wrap line if edge of screen is reached
    if (fb->cursor_x + FONT_WIDTH > fb->width) {
        fb->cursor_x = 0;
        fb->cursor_y += FONT_HEIGHT;
    }

    // Scroll if reaching bottom
    if (fb->cursor_y + FONT_HEIGHT > fb->height) {
        // Move all lines up by one character height
        memmove(fb->pixel_data, fb->pixel_data + fb->pitch_pixels * FONT_HEIGHT, (fb->height - FONT_HEIGHT) * fb->pitch_pixels * sizeof(uint32_t));
        // Clear the last line
        for (size_t y = fb->height - FONT_HEIGHT; y < fb->height; ++y) {
            for (size_t x = 0; x < fb->width; ++x) {
                framebuffer_draw_pixel(fb, x, y, 0x00000000);
            }
        }
        fb->cursor_y -= FONT_HEIGHT;
    }
}

void framebuffer_print(struct framebuffer_state* fb, const char* str, uint32_t fg_color, uint32_t bg_color) {
    while (*str) {
        framebuffer_putchar(fb, *str++, fg_color, bg_color);
    }
}
