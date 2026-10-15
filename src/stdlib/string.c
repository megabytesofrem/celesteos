/**
 * Standard C library string functions for CelesteOS.
 */

#include "string.h"

/**
 * Returns the length of the given string `str`.
 */
size_t strlen(const char* str) {
    size_t length = 0;
    while (str[length] != '\0') {
        ++length;
    }
    return length;
}

/**
 * Compares two strings `str1` and `str2` lexicographically.
 *
 * Returns zero if the strings are equal, a negative value if `str1` is less than `str2`,
 * and a positive value if `str1` is greater than `str2`.
 */
size_t strcmp(const char* str1, const char* str2) {
    while (*str1 && (*str1 == *str2)) {
        ++str1;
        ++str2;
    }
    return *(unsigned char*)str1 - *(unsigned char*)str2;
}

/**
 * Compares at most `num` characters of two strings `str1` and `str2` lexicographically.
 *
 * Returns zero if the strings are equal up to `num` characters, a negative value if `str1` is less than `str2`,
 * and a positive value if `str1` is greater than `str2`.
 */
size_t strncmp(const char* str1, const char* str2, size_t num) {
    while (num && *str1 && (*str1 == *str2)) {
        ++str1;
        ++str2;

        // Decrement the number of characters left to compare.
        --num;
    }

    // If we exited the loop because we compared `num` characters,
    // the strings are equal up to that point.
    if (num == 0) {
        return 0;
    }
    return *(unsigned char*)str1 - *(unsigned char*)str2;
}

/**
 * Concatenates the string `src` to the end of the string `dest`.
 *
 * Returns a pointer to the resulting string `dest`.
 */
char* strcat(char* dest, const char* src) {
    char* original_dest = dest;
    while (*dest) *dest++;

    while (*src) {
        *dest++ = *src++;
    }

    *dest = '\0';
    return original_dest;
}

/**
 * Copies the string `src` to the string `dest`.
 *
 * Returns a pointer to the resulting string `dest`.
 */
char* strcpy(char* dest, const char* src) {
    char* original_dest = dest;
    while (*src) {
        *dest++ = *src++;
    }
    *dest = '\0';
    return original_dest;
}

/**
 * Copies at most `num` characters of the string `src` to the string `dest`.
 *
 * Returns a pointer to the resulting string `dest`.
 */
char* strncpy(char* dest, const char* src, size_t num) {
    char* original_dest = dest;
    while (num && *src) {
        *dest++ = *src++;
        --num;
    }
    while (num) {
        *dest++ = '\0';
        --num;
    }
    return original_dest;
}

/**
 * Fills the first `num` bytes of the memory area pointed to by `ptr` with the constant byte `value`.
 *
 * Returns a pointer to the memory area `ptr`.
 */
void* memset(void* ptr, int value, size_t num) {
    unsigned char* p = (unsigned char*)ptr;
    while (num--) {
        *p++ = (unsigned char)value;
    }
    return ptr;
}

/**
 * Copies `num` bytes from memory area `src` to memory area `dest`.
 *
 * Returns a pointer to the memory area `dest`.
 */
void* memcpy(void* dest, const void* src, size_t num) {
    unsigned char* d = (unsigned char*)dest;
    const unsigned char* s = (const unsigned char*)src;
    while (num--) {
        *d++ = *s++;
    }
    return dest;
}