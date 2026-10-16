#include "framebuffer.h"

#include <stdlib/string.h>

#include "ter_u16b.h"
#include "ter_u16n.h"

struct framebuffer_state global_framebuffer;

bool framebuffer_is_initialized(struct framebuffer_state* fb) {
    return (fb != NULL && fb->fb != NULL);
}

void framebuffer_set_bold(struct framebuffer_state* fb, bool bold) {
    fb->bold = bold;
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
    uint8_t* glyph = NULL;

    // Get the glyph bitmap for the character from the font table
    // TODO: this is hacky, is there a better way?

    if (fb->bold) {
        glyph = font_8x16_b[glyph_index];
    } else {
        glyph = font_8x16_n[glyph_index];
    }

    for (size_t row = 0; row < FONT_HEIGHT; row++) {
        uint8_t bitmap_row = glyph[row];

        for (size_t col = 0; col < FONT_WIDTH; col++) {
            bool is_set = (bitmap_row & (1 << (7 - col))) != 0;
            framebuffer_draw_pixel(fb, x + col, y + row, is_set ? fg_color : bg_color);
        }
    }
}

void framebuffer_draw_raw_image(struct framebuffer_state* fb, size_t x, size_t y, const char* image_data, size_t img_width, size_t img_height) {
    const char* current_data = image_data;

    for (size_t row = 0; row < img_height; row++) {
        for (size_t column = 0; column < img_width; column++) {
            uint8_t pixel[3];

            uint8_t first = (uint8_t)current_data[0] - 33;
            uint8_t second = (uint8_t)current_data[1] - 33;
            uint8_t third = (uint8_t)current_data[2] - 33;
            uint8_t fourth = (uint8_t)current_data[3] - 33;

            pixel[0] = (uint8_t)((first << 2) | (second >> 4));
            pixel[1] = (uint8_t)(((second & 0x0F) << 4) | (third >> 2));
            pixel[2] = (uint8_t)(((third & 0x03) << 6) | fourth);
            current_data += 4;

            uint8_t r = pixel[0];
            uint8_t g = pixel[1];
            uint8_t b = pixel[2];

            // Pack into 32-bit XRGB color for Limine
            uint32_t color = ((uint32_t)(r) << 16) |
                             ((uint32_t)(g) << 8) |
                             (uint32_t)(b);

            framebuffer_draw_pixel(fb, x + column, y + row, color);
        }
    }

    // Update the cursor to after the drawn image
    fb->cursor_x = 0;
    fb->cursor_y = y + img_height + 1;
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

void framebuffer_scroll(struct framebuffer_state* fb) {
    // Copy screen lines up by FONT_HEIGHT rows using memcpy
    size_t bytes_per_line = fb->fb->pitch;
    size_t total_move_bytes = (fb->height - FONT_HEIGHT) * bytes_per_line;

    uint8_t* byte_buffer = (uint8_t*)(fb->pixel_data);
    memcpy(byte_buffer, byte_buffer + (FONT_HEIGHT * bytes_per_line), total_move_bytes);

    // Clear the newly exposed bottom row
    uint32_t* bottom_row = fb->pixel_data + ((fb->height - FONT_HEIGHT) * fb->pitch_pixels);
    memset(bottom_row, 0, FONT_HEIGHT * bytes_per_line);

    fb->cursor_y -= FONT_HEIGHT;
}