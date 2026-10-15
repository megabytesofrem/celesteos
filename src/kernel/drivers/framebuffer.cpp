#include "framebuffer.h"

#include <stdlib/string.h>

#include "font_8x16.h"
#include "pony.h"

namespace drivers {

Framebuffer& Framebuffer::instance() {
    static Framebuffer instance;
    return instance;
}

void Framebuffer::initialize(limine_framebuffer* fb) {
    this->framebuffer = fb;
    this->pixel_data = static_cast<uint32_t*>(fb->address);
    this->width = fb->width;
    this->height = fb->height;

    // Convert pixel pitch to 32-bit offset
    this->pitch_pixels = fb->pitch / sizeof(uint32_t);

    clear(0x00000000);
}

void Framebuffer::clear(uint32_t bg_color) {
    for (size_t y = 0; y < this->height; ++y) {
        for (size_t x = 0; x < this->width; ++x) {
            draw_pixel(x, y, bg_color);
        }
    }
}

void Framebuffer::draw_pixel(size_t x, size_t y, uint32_t color) {
    if (x >= this->width || y >= this->height) return;

    pixel_data[y * pitch_pixels + x] = color;
}

void Framebuffer::draw_char(char c, size_t x, size_t y, uint32_t fg_color, uint32_t bg_color) {
    auto glyph_index = static_cast<uint8_t>(c);
    const uint8_t* glyph = font_8x16[glyph_index];

    for (size_t row = 0; row < FONT_HEIGHT; row++) {
        uint8_t bitmap_row = glyph[row];

        for (size_t col = 0; col < FONT_WIDTH; col++) {
            // Check if the bit at (7 - col) is set
            bool is_set = (bitmap_row & (1 << (7 - col))) != 0;
            draw_pixel(x + col, y + row, is_set ? fg_color : bg_color);
        }
    }
}

void Framebuffer::draw_gimp_image(size_t start_x, size_t start_y, const char* data_ptr, size_t img_width, size_t img_height) {
    // Non-const pointer to iterate through the encoded ASCII data stream
    const char* current_data = data_ptr;

    for (size_t y = 0; y < img_height; y++) {
        for (size_t x = 0; x < img_width; x++) {
            uint8_t pixel[3];

            // Use GIMP's exact unpacking macro logic
            HEADER_PIXEL(current_data, pixel);

            uint8_t r = pixel[0];
            uint8_t g = pixel[1];
            uint8_t b = pixel[2];

            // Pack into 32-bit XRGB color for Limine
            uint32_t color = (static_cast<uint32_t>(r) << 16) |
                             (static_cast<uint32_t>(g) << 8) |
                             static_cast<uint32_t>(b);

            draw_pixel(start_x + x, start_y + y, color);
        }
    }

    // Update the cursor to after the drawn image
    cursor_x = 0;
    cursor_y = start_y + img_height + 1;
}

void Framebuffer::putchar(char c, uint32_t fg_color, uint32_t bg_color) {
    if (c == '\n') {
        cursor_x = 0;
        cursor_y += FONT_HEIGHT;
    } else if (c == '\r') {
        cursor_x = 0;
    } else if (c == '\t') {
        cursor_x = (cursor_x + 4 * FONT_WIDTH) & ~(4 * FONT_WIDTH - 1);
    } else {
        draw_char(c, cursor_x, cursor_y, fg_color, bg_color);
        cursor_x += FONT_WIDTH;
    }

    // Wrap line if edge of screen is reached
    if (cursor_x + FONT_WIDTH > width) {
        cursor_x = 0;
        cursor_y += FONT_HEIGHT;
    }

    // Scroll if reaching bottom
    if (cursor_y + FONT_HEIGHT > height) {
        scroll();
    }
}

void Framebuffer::print(const char* str, uint32_t fg_color, uint32_t bg_color) {
    while (*str) {
        putchar(*str++, fg_color, bg_color);
    }
}

void Framebuffer::scroll() {
    // Copy screen lines up by FONT_HEIGHT rows using memcpy
    size_t bytes_per_line = framebuffer->pitch;
    size_t total_move_bytes = (height - FONT_HEIGHT) * bytes_per_line;

    uint8_t* byte_buffer = reinterpret_cast<uint8_t*>(pixel_data);
    memcpy(byte_buffer, byte_buffer + (FONT_HEIGHT * bytes_per_line), total_move_bytes);

    // Clear the newly exposed bottom row
    uint32_t* bottom_row = pixel_data + ((height - FONT_HEIGHT) * pitch_pixels);
    memset(bottom_row, 0, FONT_HEIGHT * bytes_per_line);

    cursor_y -= FONT_HEIGHT;
}

}  // namespace drivers