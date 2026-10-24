#ifndef CELESTEOS_STRING_H
#define CELESTEOS_STRING_H

#include <stddef.h>

/**
 * Returns the length of the given string `str`.
 */
size_t strlen(const char *str);

/**
 * Compares two strings `str1` and `str2` lexicographically.
 *
 * Returns zero if the strings are equal, a negative value if `str1` is less than `str2`,
 * and a positive value if `str1` is greater than `str2`.
 */
size_t strcmp(const char *str1, const char *str2);

/**
 * Compares at most `num` characters of two strings `str1` and `str2` lexicographically.
 *
 * Returns zero if the strings are equal up to `num` characters, a negative value if `str1` is less
 * than `str2`, and a positive value if `str1` is greater than `str2`.
 */
size_t strncmp(const char *str1, const char *str2, size_t num);

/**
 * Concatenates the string `src` to the end of the string `dest`.
 *
 * Returns a pointer to the resulting string `dest`.
 */
char *strcat(char *dest, const char *src);

/**
 * Copies the string `src` to the string `dest`.
 *
 * Returns a pointer to the resulting string `dest`.
 */
char *strcpy(char *dest, const char *src);

/**
 * Copies at most `num` characters of the string `src` to the string `dest`.
 *
 * Returns a pointer to the resulting string `dest`.
 */
char *strncpy(char *dest, const char *src, size_t num);

/**
 * Fills the first `num` bytes of the memory area pointed to by `ptr` with the constant byte
 * `value`.
 *
 * Returns a pointer to the memory area `ptr`.
 */
void *memset(void *ptr, int value, size_t num);

/**
 * Compares the first `num` bytes of the memory areas `ptr1` and `ptr2`.
 *
 * Returns zero if the memory areas are equal, a negative value if `ptr1` is less than `ptr2`,
 * and a positive value if `ptr1` is greater than `ptr2`.
 */
int memcmp(const void *ptr1, const void *ptr2, size_t num);

/**
 * Copies `num` bytes from memory area `src` to memory area `dest`.
 *
 * Returns a pointer to the memory area `dest`.
 */
void *memcpy(void *destination, const void *source, size_t num);

/**
 * Copies `num` bytes from memory area `src` to memory area `dest`. The memory areas may overlap.
 *
 * Returns a pointer to the memory area `dest`.
 */
void *memmove(void *destination, const void *source, size_t num);

void itoa(int value, char *str, int base);
void utoa(unsigned int value, char *str, int base);
void ftoa(double value, char *str, int precision);

#endif // CELESTEOS_STRING_H