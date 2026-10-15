#pragma once

#include <stdint.h>

/**
 * Read a byte from the specified I/O port.
 */
static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile("inb %1, %0"
                     : "=a"(ret)
                     : "Nd"(port));
    return ret;
}

/**
 * Write a byte to the specified I/O port.
 */
static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile("outb %0, %1"
                     :
                     : "a"(value), "Nd"(port));
}

/**
 * Read a word (2 bytes) from the specified I/O port.
 */
static inline uint16_t inw(uint16_t port) {
    uint16_t ret;
    __asm__ volatile("inw %1, %0"
                     : "=a"(ret)
                     : "Nd"(port));
    return ret;
}
/**
 * Write a word (2 bytes) to the specified I/O port.
 */
static inline void outw(uint16_t port, uint16_t value) {
    __asm__ volatile("outw %0, %1"
                     :
                     : "a"(value), "Nd"(port));
}

/**
 * Read a double word (4 bytes) from the specified I/O port.
 */
static inline uint32_t inl(uint16_t port) {
    uint32_t ret;
    __asm__ volatile("inl %1, %0"
                     : "=a"(ret)
                     : "Nd"(port));
    return ret;
}

/**
 * Write a double word (4 bytes) to the specified I/O port.
 */
static inline void outl(uint16_t port, uint32_t value) {
    __asm__ volatile("outl %0, %1"
                     :
                     : "a"(value), "Nd"(port));
}

/* Debug port (Bochs/QEMU 0xE9) */

/**
 * Writes a string to the Bochs/QEMU debug port (0xE9).
 */
static inline void e9_write(const char* str) {
    while (*str != '\0') {
        outb(0xE9, *str++);
    }
}
