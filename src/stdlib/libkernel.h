#ifndef CELESTEOS_STD_LIBKERNEL_H
#define CELESTEOS_STD_LIBKERNEL_H

#include <stdint.h>
#include <stdbool.h>

enum log_level {
    KLOG_NONE,
    KLOG_INFO,
    KLOG_WARN,
    KLOG_ERROR,
};

void printk(const char* format,
            uint32_t fg_color,
            uint32_t bg_color,
            ...);
void kputs(const char* str,
           uint32_t fg_color,
           uint32_t bg_color,
           bool bold);

void klog(enum log_level level, const char* format, ...);

#endif  // CELESTEOS_STD_LIBKERNEL_H