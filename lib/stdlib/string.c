#include "string.h"

#include <stdint.h>

size_t strlen(const char *str) {
    size_t length = 0;
    while (str[length] != '\0') {
        ++length;
    }
    return length;
}

size_t strcmp(const char *str1, const char *str2) {
    while (*str1 && (*str1 == *str2)) {
        ++str1;
        ++str2;
    }
    return *(unsigned char *)str1 - *(unsigned char *)str2;
}

size_t strncmp(const char *str1, const char *str2, size_t num) {
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
    return *(unsigned char *)str1 - *(unsigned char *)str2;
}

char *strcat(char *dest, const char *src) {
    char *original_dest = dest;
    while (*dest != '\0') {
        ++dest;
    }

    while (*src) {
        *dest++ = *src++;
    }

    *dest = '\0';
    return original_dest;
}

char *strcpy(char *dest, const char *src) {
    char *original_dest = dest;
    while (*src) {
        *dest++ = *src++;
    }
    *dest = '\0';
    return original_dest;
}

char *strncpy(char *dest, const char *src, size_t num) {
    char *original_dest = dest;
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

void *memset(void *ptr, int value, size_t num) {
    unsigned char *p = (unsigned char *)ptr;
    while (num--) {
        *p++ = (unsigned char)value;
    }
    return ptr;
}

int memcmp(const void *ptr1, const void *ptr2, size_t num) {
    const unsigned char *p1 = (const unsigned char *)ptr1;
    const unsigned char *p2 = (const unsigned char *)ptr2;
    while (num--) {
        if (*p1 != *p2) {
            return *p1 - *p2;
        }
        ++p1;
        ++p2;
    }
    return 0;
}

void *memcpy(void *dest, const void *src, size_t num) {
    unsigned char *d = (unsigned char *)dest;
    const unsigned char *s = (const unsigned char *)src;
    while (num--) {
        *d++ = *s++;
    }
    return dest;
}

void *memmove(void *dest, const void *src, size_t num) {
    unsigned char *d = (unsigned char *)dest;
    const unsigned char *s = (const unsigned char *)src;

    if ((uintptr_t)d < (uintptr_t)s) {
        for (size_t i = 0; i < num; ++i) {
            d[i] = s[i];
        }
    } else if ((uintptr_t)d > (uintptr_t)s) {
        while (num > 0) {
            --num;
            d[num] = s[num];
        }
    }
    return dest;
}

void itoa(int value, char *str, int base) {
    int i;
    int sign;

    if ((sign = value) < 0) {
        value = -value;
    }

    i = 0;

    do {
        str[i++] = (value % base) + '0';
    } while ((value /= base) > 0);

    if (sign < 0) {
        str[i++] = '-';
    }

    str[i] = '\0';

    // Reverse the string
    int start = 0;
    int end = i - 1;
    while (start < end) {
        char temp = str[start];
        str[start] = str[end];
        str[end] = temp;
        ++start;
        --end;
    }
}

void utoa(unsigned int value, char *str, int base) {
    int i = 0;

    do {
        str[i++] = "0123456789abcdefghijklmnopqrstuvwxyz"[value % base];
    } while ((value /= base) > 0);

    str[i] = '\0';

    // Reverse the string
    int start = 0;
    int end = i - 1;
    while (start < end) {
        char temp = str[start];
        str[start] = str[end];
        str[end] = temp;
        ++start;
        --end;
    }
}

void ftoa(double value, char *str, int precision) {
    // Handle negative numbers
    if (value < 0) {
        *str++ = '-';
        value = -value;
    }

    // Extract integer part
    int int_part = (int)value;
    itoa(int_part, str, 10);

    // Move the pointer to the end of the integer part
    while (*str != '\0') {
        ++str;
    }

    // Add the decimal point
    *str++ = '.';

    // Extract fractional part
    double frac_part = value - int_part;
    for (int i = 0; i < precision; ++i) {
        frac_part *= 10;
        int digit = (int)frac_part;
        *str++ = '0' + digit;
        frac_part -= digit;
    }

    // Null-terminate the string
    *str = '\0';
}