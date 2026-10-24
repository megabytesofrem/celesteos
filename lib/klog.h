#ifndef CELESTEOS_KLOG_H
#define CELESTEOS_KLOG_H

#include <stdbool.h>
#include <stdint.h>

enum log_level {
    KLOG_NONE,
    KLOG_INFO,
    KLOG_DEBUG,
    KLOG_WARN,
    KLOG_ERROR,
};

/**
 * Kernel formatted print function.
 */
void kprintf(const char *format, ...);

/**
 * Kernel puts function.
 */
void kputs(const char *str);

/**
 * Kernel puts function with color and bold options.
 */
void kputs_color(const char *str, uint32_t fg_color, uint32_t bg_color, bool bold);

/**
 * Kernel logging function.
 */
void klog(enum log_level level, const char *format, ...);

#endif // CELESTEOS_KLOG_H