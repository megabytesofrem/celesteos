#ifndef LIBC_STRING_H
#define LIBC_STRING_H

#include "common.h"

size_t strlen(const char *s);
size_t strcmp(const char *s1, const char *s2);
char *strcat(char *dst, const char *src);
char *strcpy(char *dst, const char *src);
char *strchr(const char *s, int c);

#endif // LIBC_STRING_H