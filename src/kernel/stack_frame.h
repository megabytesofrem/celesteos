#ifndef CELESTEOS_STACK_FRAME_H
#define CELESTEOS_STACK_FRAME_H

#include <stdint.h>

typedef struct __attribute__((packed)) stack_frame {
    uint64_t rax; /* syscall number, negative if mach trap */
    uint64_t rbx;
    uint64_t rdi; /* argument 1 */
    uint64_t rsi; /* argument 2 */
    uint64_t rdx; /* argument 3 */
    uint64_t r8;  /* argument 5 */
    uint64_t r9;  /* argument 6 */
    uint64_t r10; /* argument 4 (XNU System V override) */
    uint64_t r11; /* saved scratch register */
    uint64_t r12;
    uint64_t r13;
    uint64_t r14;
    uint64_t r15;
    uint64_t rbp;
    uint64_t rsp;    /* user return RSP */
    uint64_t rip;    /* user return RIP */
    uint64_t rflags; /* user return RFLAGS */
} stack_frame_t;

#endif /* CELESTEOS_STACK_FRAME_H */