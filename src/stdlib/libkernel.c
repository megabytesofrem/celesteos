/**
 * Common kernel library functions
 */

#include "libkernel.h"

#include <cpu/ports.h>
#include <drivers/framebuffer.h>

#include <stdarg.h>
#include "string.h"

void _kprintc(char c,
              uint32_t fg_color,
              uint32_t bg_color) {
    outb(0xE9, c);

    if (framebuffer_is_initialized(&global_framebuffer)) {
        framebuffer_putchar(&global_framebuffer, c, fg_color, bg_color);
    }
}

void kputs(const char* str, uint32_t fg_color, uint32_t bg_color) {
    while (*str) {
        _kprintc(*str++, fg_color, bg_color);
    }
}

static void vkprint(const char* format,
                    uint32_t fg_color,
                    uint32_t bg_color,
                    va_list args) {
    int i = 0;

    while (format[i]) {
        if (format[i] == '%') {
            i++;
            if (format[i] == 'c') {
                // %c: format a single character
                char c = (char)va_arg(args, int);
                _kprintc(c, fg_color, bg_color);
            } else if (format[i] == 's') {
                // %s: format a string
                const char* s = va_arg(args, const char*);
                kputs(s, fg_color, bg_color);
            } else if (format[i] == 'd') {
                // %d: format an integer as a decimal number
                int d = va_arg(args, int);
                char buffer[12];
                itoa(d, buffer, 10);
                kputs(buffer, fg_color, bg_color);
            } else if (format[i] == 'f') {
                // %f: format a floating-point number
                double f = va_arg(args, double);
                char buffer[32];
                ftoa(f, buffer, 6);  // Assuming ftoa is implemented
                kputs(buffer, fg_color, bg_color);
            } else if (format[i] == 'x') {
                // %x: format an integer as a hexadecimal number
                unsigned int x = va_arg(args, unsigned int);
                char buffer[12];
                utoa(x, buffer, 16);
                kputs(buffer, fg_color, bg_color);
            } else if (format[i] == 'p') {
                // %p: format a pointer as a hexadecimal address
                uintptr_t address = (uintptr_t)(va_arg(args, void*));
                for (int shift = sizeof(uintptr_t) * 8 - 4; shift >= 0;
                     shift -= 4) {
                    _kprintc("0123456789abcdef"[(address >> shift) & 0xF],
                             fg_color, bg_color);
                }
            } else if (format[i] == 'u') {
                // %u: format an unsigned integer as a decimal number
                unsigned int u = va_arg(args, unsigned int);
                char buffer[12];
                utoa(u, buffer, 10);
                kputs(buffer, fg_color, bg_color);
            } else if (format[i] == '%') {
                // %%: format a literal '%' character
                _kprintc('%', fg_color, bg_color);
            }
        } else {
            _kprintc(format[i], fg_color, bg_color);
        }
        i++;
    }
}

void printk(const char* format, uint32_t fg_color, uint32_t bg_color, ...) {
    va_list args;
    va_start(args, bg_color);
    vkprint(format, fg_color, bg_color, args);
    va_end(args);
}

void klog(enum log_level level, const char* format, ...) {
    va_list args;
    va_start(args, format);

    switch (level) {
        case KLOG_INFO:
            kputs("[INFO] ", 0xb5b5b3, 0x00000000);
            break;
        case KLOG_WARN:
            kputs("[WARN] ", 0xfff952, 0x00000000);
            break;
        case KLOG_ERROR:
            kputs("[ERROR] ", 0xe33b3b, 0x00000000);
            break;
        default:
            kputs("[UNKNOWN] ", 0xb5b5b3, 0x00000000);  // Unknown
            break;
    }

    vkprint(format, 0xFFFFFFFF, 0x00000000, args);

    va_end(args);
}