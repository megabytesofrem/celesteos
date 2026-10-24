#ifndef CELESTEOS_KERNEL_FB_H
#define CELESTEOS_KERNEL_FB_H

#include <limine.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct framebuffer_state {
    struct limine_framebuffer *fb;

    bool bold;
    uint32_t *pixel_data;

    size_t width;
    size_t height;
    size_t pitch_pixels;
    size_t cursor_x;
    size_t cursor_y;
};

extern struct framebuffer_state global_framebuffer;

bool fb_is_initialized(struct framebuffer_state *fb);

void fb_set_bold(struct framebuffer_state *fb, bool bold);
struct framebuffer_state *fb_get();

void fb_init(struct framebuffer_state *fb, struct limine_framebuffer *limine_fb);
void fb_clear(struct framebuffer_state *fb, uint32_t bg_color);
void fb_draw_pixel(struct framebuffer_state *fb, size_t x, size_t y, uint32_t color);
void fb_draw_char(
    struct framebuffer_state *fb, char c, size_t x, size_t y, uint32_t fg_color, uint32_t bg_color);
void fb_draw_raw_image(struct framebuffer_state *fb, size_t x, size_t y, const char *image_data,
    size_t img_width, size_t img_height);
void fb_putchar(struct framebuffer_state *fb, char c, uint32_t fg_color, uint32_t bg_color);
void fb_print(struct framebuffer_state *fb, const char *str, uint32_t fg_color, uint32_t bg_color);
void fb_scroll(struct framebuffer_state *fb);

#endif /* CELESTEOS_KERNEL_FB_H */