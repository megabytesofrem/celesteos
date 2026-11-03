#include <ke/log.h>

#include <ke/fb.h>
#include <ke/ports.h>

#include "string.h"
#include <stdarg.h>

static void KePrintCharacter(
    char character, uint32_t foregroundColor, uint32_t backgroundColor, bool bold) {
    outb(0xE9, character);

    if (KeFbIsInitialized(&g_keFramebuffer)) {
        KeFbSetBold(&g_keFramebuffer, bold);
        KeFbPutCharacter(&g_keFramebuffer, character, foregroundColor, backgroundColor);
    }
}

void KePrintString(const char *string) {
    while (*string) {
        KePrintCharacter(*string++, 0xFFFFFFFF, 0x00000000, false);
    }
}

void KePrintStringColor(
    const char *string, uint32_t foregroundColor, uint32_t backgroundColor, bool bold) {
    while (*string) {
        KePrintCharacter(*string++, foregroundColor, backgroundColor, bold);
    }
}

static void KeVPrint(
    const char *format, uint32_t foregroundColor, uint32_t backgroundColor, va_list args) {
    int i = 0;

    while (format[i]) {
        if (format[i] == '%') {
            i++;
            if (format[i] == 'c') {
                // %c: format a single character
                char c = (char)va_arg(args, int);
                KePrintCharacter(c, foregroundColor, backgroundColor, false);
            } else if (format[i] == 's') {
                // %s: format a string
                const char *s = va_arg(args, const char *);
                KePrintStringColor(s, foregroundColor, backgroundColor, false);
            } else if (format[i] == 'd') {
                // %d: format an integer as a decimal number
                int d = va_arg(args, int);
                char buffer[12];
                itoa(d, buffer, 10);
                KePrintStringColor(buffer, foregroundColor, backgroundColor, false);
            } else if (format[i] == 'f') {
                // %f: format a floating-point number
                double f = va_arg(args, double);
                char buffer[32];
                ftoa(f, buffer, 6); // Assuming ftoa is implemented
                KePrintStringColor(buffer, foregroundColor, backgroundColor, false);
            } else if (format[i] == 'x') {
                // %x: format an integer as a hexadecimal number
                unsigned int x = va_arg(args, unsigned int);
                char buffer[12];
                utoa(x, buffer, 16);
                KePrintStringColor(buffer, foregroundColor, backgroundColor, false);
            } else if (format[i] == 'p') {
                // %p: format a pointer as a hexadecimal address
                uintptr_t address = (uintptr_t)(va_arg(args, void *));
                for (int shift = sizeof(uintptr_t) * 8 - 4; shift >= 0; shift -= 4) {
                    KePrintCharacter("0123456789abcdef"[(address >> shift) & 0xF], foregroundColor,
                        backgroundColor, false);
                }
            } else if (format[i] == 'u') {
                // %u: format an unsigned integer as a decimal number
                unsigned int u = va_arg(args, unsigned int);
                char buffer[12];
                utoa(u, buffer, 10);
                KePrintStringColor(buffer, foregroundColor, backgroundColor, false);
            } else if (format[i] == '%') {
                // %%: format a literal '%' character
                KePrintCharacter('%', foregroundColor, backgroundColor, false);
            }
        } else {
            KePrintCharacter(format[i], foregroundColor, backgroundColor, false);
        }
        i++;
    }
}

void KePrint(const char *format, ...) {
    va_list args;
    va_start(args, format);
    KeVPrint(format, 0xFFFFFFFF, 0x00000000, args);
    va_end(args);
}

void KeLog(KeLogLevel level, const char *format, ...) {
    va_list args;
    va_start(args, format);

    switch (level) {
    case KE_LOG_INFO:
        KePrintStringColor("[INFO] ", 0xa3a3a3, 0x00000000, true);
        break;
    case KE_LOG_WARN:
        KePrintStringColor("[WARN] ", 0xfff952, 0x00000000, true);
        break;
    case KE_LOG_ERROR:
        KePrintStringColor("[ERROR] ", 0xe33b3b, 0x00000000, true);
        break;
    case KE_LOG_DEBUG:
        KePrintStringColor("[DEBUG] ", 0x52dce3, 0x00000000, true);
        break;
    default:
        KePrintStringColor("[UNKNOWN] ", 0xb5b5b3, 0x00000000, true); // Unknown
        break;
    }

    KeFbSetBold(&g_keFramebuffer, false);
    KeVPrint(format, 0xFFFFFFFF, 0x00000000, args);

    va_end(args);
}