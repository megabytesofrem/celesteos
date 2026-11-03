#ifndef CELESTEOS_KE_LOG_H
#define CELESTEOS_KE_LOG_H

#include <stdbool.h>
#include <stdint.h>

typedef enum KeLogLevel {
    KE_LOG_NONE,
    KE_LOG_INFO,
    KE_LOG_DEBUG,
    KE_LOG_WARN,
    KE_LOG_ERROR,
} KeLogLevel;

/**
 * Print a formatted string to the kernel output.
 */
void KePrint(const char *format, ...);

/**
 * Print a string to the kernel output.
 */
void KePrintString(const char *string);

/**
 * Print a string to the kernel output with color and bold options.
 */
void KePrintStringColor(
    const char *string, uint32_t foregroundColor, uint32_t backgroundColor, bool bold);

/**
 * Kernel logging function.
 */
void KeLog(KeLogLevel level, const char *format, ...);

#endif // CELESTEOS_KE_LOG_H