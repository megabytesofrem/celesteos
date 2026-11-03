#include "fb.h"

#include <string.h>

#include <ter_u16b.h>
#include <ter_u16n.h>

KeFramebufferState g_keFramebuffer;

bool KeFbIsInitialized(const KeFramebufferState *framebuffer) {
    return framebuffer != NULL && framebuffer->framebuffer != NULL;
}

void KeFbSetBold(KeFramebufferState *framebuffer, bool bold) {
    framebuffer->bold = bold;
}

KeFramebufferState *KeFbGet(void) {
    return &g_keFramebuffer;
}

void KeFbInitialize(KeFramebufferState *framebuffer, struct limine_framebuffer *limineFramebuffer) {
    framebuffer->framebuffer = limineFramebuffer;
    framebuffer->pixelData = (uint32_t *)limineFramebuffer->address;
    framebuffer->width = limineFramebuffer->width;
    framebuffer->height = limineFramebuffer->height;
    framebuffer->pitchPixels = limineFramebuffer->pitch / sizeof(uint32_t);
    framebuffer->cursorX = 0;
    framebuffer->cursorY = 0;

    KeFbClear(framebuffer, 0x00000000);
}

void KeFbClear(KeFramebufferState *framebuffer, uint32_t backgroundColor) {
    for (size_t y = 0; y < framebuffer->height; ++y) {
        for (size_t x = 0; x < framebuffer->width; ++x) {
            KeFbDrawPixel(framebuffer, x, y, backgroundColor);
        }
    }
}

void KeFbDrawPixel(KeFramebufferState *framebuffer, size_t x, size_t y, uint32_t color) {
    if (x >= framebuffer->width || y >= framebuffer->height)
        return;

    framebuffer->pixelData[y * framebuffer->pitchPixels + x] = color;
}

void KeFbDrawCharacter(KeFramebufferState *framebuffer, char character, size_t x, size_t y,
    uint32_t foregroundColor, uint32_t backgroundColor) {
    uint8_t glyph_index = (uint8_t)character;
    uint8_t *glyph = NULL;

    // Get the glyph bitmap for the character from the font table
    // TODO: this is hacky, is there a better way?

    if (framebuffer->bold) {
        glyph = (uint8_t *)(font_8x16_b[glyph_index]);
    } else {
        glyph = (uint8_t *)(font_8x16_n[glyph_index]);
    }

    for (size_t row = 0; row < FONT_HEIGHT; row++) {
        uint8_t bitmap_row = glyph[row];

        for (size_t col = 0; col < FONT_WIDTH; col++) {
            bool is_set = (bitmap_row & (1 << (7 - col))) != 0;
            KeFbDrawPixel(
                framebuffer, x + col, y + row, is_set ? foregroundColor : backgroundColor);
        }
    }
}

void KeFbDrawRawImage(KeFramebufferState *framebuffer, size_t x, size_t y, const char *imageData,
    size_t imageWidth, size_t imageHeight) {
    const char *currentData = imageData;

    for (size_t row = 0; row < imageHeight; row++) {
        for (size_t column = 0; column < imageWidth; column++) {
            uint8_t pixel[3];

            uint8_t first = (uint8_t)currentData[0] - 33;
            uint8_t second = (uint8_t)currentData[1] - 33;
            uint8_t third = (uint8_t)currentData[2] - 33;
            uint8_t fourth = (uint8_t)currentData[3] - 33;

            pixel[0] = (uint8_t)((first << 2) | (second >> 4));
            pixel[1] = (uint8_t)(((second & 0x0F) << 4) | (third >> 2));
            pixel[2] = (uint8_t)(((third & 0x03) << 6) | fourth);
            currentData += 4;

            uint8_t r = pixel[0];
            uint8_t g = pixel[1];
            uint8_t b = pixel[2];

            // Pack into 32-bit XRGB color for Limine
            uint32_t color = ((uint32_t)(r) << 16) | ((uint32_t)(g) << 8) | (uint32_t)(b);

            KeFbDrawPixel(framebuffer, x + column, y + row, color);
        }
    }

    // Update the cursor to after the drawn image
    framebuffer->cursorX = 0;
    framebuffer->cursorY = y + imageHeight + 1;
}

void KeFbPutCharacter(KeFramebufferState *framebuffer, char character, uint32_t foregroundColor,
    uint32_t backgroundColor) {
    if (character == '\n') {
        framebuffer->cursorX = 0;
        framebuffer->cursorY += FONT_HEIGHT;
    } else if (character == '\r') {
        framebuffer->cursorX = 0;
    } else if (character == '\t') {
        framebuffer->cursorX = (framebuffer->cursorX + 4 * FONT_WIDTH) & ~(4 * FONT_WIDTH - 1);
    } else {
        KeFbDrawCharacter(framebuffer, character, framebuffer->cursorX, framebuffer->cursorY,
            foregroundColor, backgroundColor);
        framebuffer->cursorX += FONT_WIDTH;
    }

    // Wrap line if edge of screen is reached
    if (framebuffer->cursorX + FONT_WIDTH > framebuffer->width) {
        framebuffer->cursorX = 0;
        framebuffer->cursorY += FONT_HEIGHT;
    }

    // Scroll if reaching bottom
    if (framebuffer->cursorY + FONT_HEIGHT > framebuffer->height) {
        // Move all lines up by one character height
        memmove(framebuffer->pixelData,
            framebuffer->pixelData + framebuffer->pitchPixels * FONT_HEIGHT,
            (framebuffer->height - FONT_HEIGHT) * framebuffer->pitchPixels * sizeof(uint32_t));
        // Clear the last line
        for (size_t y = framebuffer->height - FONT_HEIGHT; y < framebuffer->height; ++y) {
            for (size_t x = 0; x < framebuffer->width; ++x) {
                KeFbDrawPixel(framebuffer, x, y, 0x00000000);
            }
        }
        framebuffer->cursorY -= FONT_HEIGHT;
    }
}

void KeFbPrint(KeFramebufferState *framebuffer, const char *string, uint32_t foregroundColor,
    uint32_t backgroundColor) {
    while (*string) {
        KeFbPutCharacter(framebuffer, *string++, foregroundColor, backgroundColor);
    }
}

void KeFbScroll(KeFramebufferState *framebuffer) {
    // Copy screen lines up by FONT_HEIGHT rows using memcpy
    size_t bytes_per_line = framebuffer->framebuffer->pitch;
    size_t total_move_bytes = (framebuffer->height - FONT_HEIGHT) * bytes_per_line;

    uint8_t *byte_buffer = (uint8_t *)(framebuffer->pixelData);
    memcpy(byte_buffer, byte_buffer + (FONT_HEIGHT * bytes_per_line), total_move_bytes);

    // Clear the newly exposed bottom row
    uint32_t *bottom_row =
        framebuffer->pixelData + ((framebuffer->height - FONT_HEIGHT) * framebuffer->pitchPixels);
    memset(bottom_row, 0, FONT_HEIGHT * bytes_per_line);

    framebuffer->cursorY -= FONT_HEIGHT;
}