#ifndef CELESTEOS_STD_STRING_H
#define CELESTEOS_STD_STRING_H

#include <cstddef>

extern "C" {

size_t strlen(const char* str);
size_t strcmp(const char* str1, const char* str2);
size_t strncmp(const char* str1, const char* str2, size_t num);
char* strcat(char* dest, const char* src);
char* strcpy(char* dest, const char* src);
char* strncpy(char* dest, const char* src, size_t num);

void* memset(void* ptr, int value, size_t num);
void* memcpy(void* destination, const void* source, size_t num);
}

#endif  // CELESTEOS_STD_STRING_H