#include "libkernel.h"

#include <drivers/framebuffer.h>
#include <kernel/ports.h>

#include "string.h"
#include <stdarg.h>

void _kprintc(char c, uint32_t fg_color, uint32_t bg_color, bool bold) {
    outb(0xE9, c);

    if (framebuffer_is_initialized(&global_framebuffer)) {
        framebuffer_set_bold(&global_framebuffer, bold);
        framebuffer_putchar(&global_framebuffer, c, fg_color, bg_color);
    }
}

void kputs(const char *str) {
    while (*str) {
        _kprintc(*str++, 0xFFFFFFFF, 0x00000000, false);
    }
}

void kputs_color(const char *str, uint32_t fg_color, uint32_t bg_color, bool bold) {
    while (*str) {
        _kprintc(*str++, fg_color, bg_color, bold);
    }
}

static void vkprint(const char *format, uint32_t fg_color, uint32_t bg_color, va_list args) {
    int i = 0;

    while (format[i]) {
        if (format[i] == '%') {
            i++;
            if (format[i] == 'c') {
                // %c: format a single character
                char c = (char)va_arg(args, int);
                _kprintc(c, fg_color, bg_color, false);
            } else if (format[i] == 's') {
                // %s: format a string
                const char *s = va_arg(args, const char *);
                kputs_color(s, fg_color, bg_color, false);
            } else if (format[i] == 'd') {
                // %d: format an integer as a decimal number
                int d = va_arg(args, int);
                char buffer[12];
                itoa(d, buffer, 10);
                kputs_color(buffer, fg_color, bg_color, false);
            } else if (format[i] == 'f') {
                // %f: format a floating-point number
                double f = va_arg(args, double);
                char buffer[32];
                ftoa(f, buffer, 6); // Assuming ftoa is implemented
                kputs_color(buffer, fg_color, bg_color, false);
            } else if (format[i] == 'x') {
                // %x: format an integer as a hexadecimal number
                unsigned int x = va_arg(args, unsigned int);
                char buffer[12];
                utoa(x, buffer, 16);
                kputs_color(buffer, fg_color, bg_color, false);
            } else if (format[i] == 'p') {
                // %p: format a pointer as a hexadecimal address
                uintptr_t address = (uintptr_t)(va_arg(args, void *));
                for (int shift = sizeof(uintptr_t) * 8 - 4; shift >= 0; shift -= 4) {
                    _kprintc("0123456789abcdef"[(address >> shift) & 0xF], fg_color, bg_color,
                             false);
                }
            } else if (format[i] == 'u') {
                // %u: format an unsigned integer as a decimal number
                unsigned int u = va_arg(args, unsigned int);
                char buffer[12];
                utoa(u, buffer, 10);
                kputs_color(buffer, fg_color, bg_color, false);
            } else if (format[i] == '%') {
                // %%: format a literal '%' character
                _kprintc('%', fg_color, bg_color, false);
            }
        } else {
            _kprintc(format[i], fg_color, bg_color, false);
        }
        i++;
    }
}

void kprintf(const char *format, ...) {
    va_list args;
    va_start(args, format);
    vkprint(format, 0xFFFFFFFF, 0x00000000, args);
    va_end(args);
}

void klog(enum log_level level, const char *format, ...) {
    va_list args;
    va_start(args, format);

    switch (level) {
    case KLOG_INFO:
        kputs_color("[INFO] ", 0xa3a3a3, 0x00000000, true);
        break;
    case KLOG_WARN:
        kputs_color("[WARN] ", 0xfff952, 0x00000000, true);
        break;
    case KLOG_ERROR:
        kputs_color("[ERROR] ", 0xe33b3b, 0x00000000, true);
        break;
    default:
        kputs_color("[UNKNOWN] ", 0xb5b5b3, 0x00000000, true); // Unknown
        break;
    }

    framebuffer_set_bold(&global_framebuffer, false);
    vkprint(format, 0xFFFFFFFF, 0x00000000, args);

    va_end(args);
}