#include "cpuid.h"

#include <stdint.h>

void cpuid(int code, uint32_t *eax, uint32_t *edx) {
    __asm__ volatile("cpuid" : "=a"(*eax), "=d"(*edx) : "a"(code) : "ecx", "ebx");
}

int cpuid_string(int code, uint32_t where[4]) {
    __asm__ volatile("cpuid"
                     : "=a"(*where), "=b"(*(where + 1)), "=c"(*(where + 2)), "=d"(*(where + 3))
                     : "a"(code));
    return (int)where[0];
}