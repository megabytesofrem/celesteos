#ifndef CELESTEOS_STD_LIBKERNEL_H
#define CELESTEOS_STD_LIBKERNEL_H

#include <stdbool.h>
#include <stdint.h>

enum log_level {
	KLOG_NONE,
	KLOG_INFO,
	KLOG_WARN,
	KLOG_ERROR,
};

/**
 * Kernel formatted print function.
 */
void printk(const char *format, uint32_t fg_color, uint32_t bg_color, ...);

/**
 * Kernel puts function.
 */
void kputs(const char *str, uint32_t fg_color, uint32_t bg_color, bool bold);

/**
 * Kernel logging function.
 */
void klog(enum log_level level, const char *format, ...);

#endif // CELESTEOS_STD_LIBKERNEL_H