#ifndef CELESTEOS_FRAMEBUFFER_H
#define CELESTEOS_FRAMEBUFFER_H

#include <limine.h>

#include <cstddef>
#include <cstdint>

namespace drivers {

class Framebuffer {
   private:
    Framebuffer() = default;

    limine_framebuffer* framebuffer = nullptr;
    uint32_t* pixel_data = nullptr;

    // Size of the framebuffer
    size_t width = 0;
    size_t height = 0;
    size_t pitch_pixels = 0; /* number of pixels per row */
    size_t cursor_x = 0;
    size_t cursor_y = 0;

    static constexpr size_t FONT_WIDTH = 8;
    static constexpr size_t FONT_HEIGHT = 16;

    void scroll();

   public:
    Framebuffer(const Framebuffer&) = delete;
    Framebuffer& operator=(const Framebuffer&) = delete;
    ~Framebuffer() = default;

    static Framebuffer& instance();

    [[nodiscard]] bool is_initialized() const { return pixel_data != nullptr; }

    void initialize(limine_framebuffer* fb);
    void clear(uint32_t bg_color = 0x00000000);
    void draw_pixel(size_t x, size_t y, uint32_t color);
    void draw_char(char c, size_t x, size_t y, uint32_t fg_color = 0xFFFFFFFF, uint32_t bg_color = 0x00000000);
    void putchar(char c, uint32_t fg_color = 0xFFFFFFFF, uint32_t bg_color = 0x00000000);
    void print(const char* str, uint32_t fg_color = 0xFFFFFFFF, uint32_t bg_color = 0x00000000);
};

}  // namespace drivers

#endif  // CELESTEOS_FRAMEBUFFER_H