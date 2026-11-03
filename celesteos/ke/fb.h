#ifndef CELESTEOS_FB_H
#define CELESTEOS_FB_H

#include <limine.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct KeFramebufferState {
    struct limine_framebuffer *framebuffer;
    bool bold;
    uint32_t *pixelData;
    size_t width;
    size_t height;
    size_t pitchPixels;
    size_t cursorX;
    size_t cursorY;
} KeFramebufferState;

extern KeFramebufferState g_keFramebuffer;

bool KeFbIsInitialized(const KeFramebufferState *framebuffer);
void KeFbSetBold(KeFramebufferState *framebuffer, bool bold);
KeFramebufferState *KeFbGet(void);
void KeFbInitialize(KeFramebufferState *framebuffer, struct limine_framebuffer *limineFramebuffer);
void KeFbClear(KeFramebufferState *framebuffer, uint32_t backgroundColor);
void KeFbDrawPixel(KeFramebufferState *framebuffer, size_t x, size_t y, uint32_t color);
void KeFbDrawCharacter(KeFramebufferState *framebuffer, char character, size_t x, size_t y,
    uint32_t foregroundColor, uint32_t backgroundColor);
void KeFbDrawRawImage(KeFramebufferState *framebuffer, size_t x, size_t y, const char *imageData,
    size_t imageWidth, size_t imageHeight);
void KeFbPutCharacter(KeFramebufferState *framebuffer, char character, uint32_t foregroundColor,
    uint32_t backgroundColor);
void KeFbPrint(KeFramebufferState *framebuffer, const char *string, uint32_t foregroundColor,
    uint32_t backgroundColor);
void KeFbScroll(KeFramebufferState *framebuffer);

#endif // CELESTEOS_FB_H
