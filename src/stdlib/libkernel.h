#ifndef CELESTEOS_STD_LIBKERNEL_H
#define CELESTEOS_STD_LIBKERNEL_H

#include <cstdint>

extern "C" {

enum LogLevel {
    LOG_NONE,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR,
};

void printk(const char* format,
            uint32_t fg_color = 0xFFFFFFFF,
            uint32_t bg_color = 0x00000000,
            ...);
void kputs(const char* str,
           uint32_t fg_color = 0xFFFFFFFF,
           uint32_t bg_color = 0x00000000);
void klog(enum LogLevel level, const char* format, ...);
}

#endif  // CELESTEOS_STD_LIBKERNEL_H