/**
 * Common kernel library functions
 */

#include "libkernel.h"

#include <cpu/ports.h>

#include <cstdarg>

#include "string.h"

void _kprintc(char c) {
    // TODO: Write to the QEMU debug port (0xE9)
    outb(0xE9, c);
}

void _kputs(const char* str) {
    while (*str) {
        _kprintc(*str++);
    }
}

void kputs(char* str) {
    size_t len = strlen(str);
    str[len] = '\0';

    _kputs(str);
}

void printk(const char* format, ...) {
    int i = 0;

    va_list args;
    va_start(args, format);

    while (format[i]) {
        if (format[i] == '%') {
            i++;
            if (format[i] == 'c') {
                char c = (char)va_arg(args, int);
                _kprintc(c);
            } else if (format[i] == 's') {
                const char* s = va_arg(args, const char*);
                _kputs(s);
            } else if (format[i] == 'd') {
                // TODO: handle integer formatting for %d
            }
        } else {
            _kprintc(format[i]);
        }
        i++;
    }

    va_end(args);
}

void klog(LogLevel level, const char* format, ...) {
    // stub
    va_list args;
    va_start(args, format);

    printk(format, args);

    va_end(args);
}