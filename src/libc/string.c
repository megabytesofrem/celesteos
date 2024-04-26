/**
 * string.c: string library functions
 */
#include "string.h"

/**
 * strlen: Calculate the length of `s`
 */
size_t strlen(const char *s) {
    size_t len = 0;
    while (*s++ != '\0')
        len++;

    return len;
}

/**
 * strcmp: Compare two strings `s1` and `s2`
 */
size_t strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }

    return *(const unsigned char *)s1 - *(const unsigned char *)s2;
}

/**
 * strncmp: Compare two strings `s1` and `s2` up to `n` characters
 */
size_t strncmp(const char *s1, const char *s2, size_t n) {
    while (n-- && *s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }

    return *(const unsigned char *)s1 - *(const unsigned char *)s2;
}

/**
 * strcat: Concatenate `src` to `dst`
 */
char *strcat(char *dst, const char *src) {
    char *ret = dst;
    while (*dst)
        dst++;

    // Copy src to dst
    while (*dst++ = *src++)
        ;
    return ret;
}

/**
 * strcpy: Copy `src` to `dst`
 */
char *strcpy(char *dst, const char *src) {
    char *tmp = dst;
    while (*src)
        *dst++ = *src++;

    *dst = '\0';
    return tmp;
}

/**
 * strncpy: Copy at most up to `n` characters from `src` to `dst`
 */
char *strncpy(char *dst, const char *src, size_t n) {
    char *tmp = dst;
    while (n) {
        if ((*dst = *src) != 0)
            src++;
        tmp++;
        n--;
    }

    return dst;
}

/**
 * strchr: Find the first occurrence of `c` in `s`
 */
char *strchr(const char *s, int c) {
    while (*s) {
        if (*s == c)
            // Found a match
            return (char *)s;
        s++;
    }

    return NULL;
}

/**
 * memcpy: Copy `n` bytes from `src` to `dst`
 */
void *memcpy(void *dst, const void *src, size_t n) {
    char *tmp = dst;
    const char *s = src;

    while (n--)
        *tmp++ = *s++;
    return dst;
}

/**
 * memset: Set `n` bytes in `dst` to `c`
 */
void *memset(void *dst, int c, size_t n) {
    char *tmp = dst;
    while (n--)
        *tmp++ = c;
    return dst;
}