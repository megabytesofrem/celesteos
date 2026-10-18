#ifndef CELESTEOS_SYSCALL_H
#define CELESTEOS_SYSCALL_H

#include <stdint.h>

#define IA32_EFER_MSR 0xC0000080  /* EFER: Extended Feature Enable Register */
#define IA32_STAR_MSR 0xC0000081  /* STAR: Segment Target Address Register */
#define IA32_LSTAR_MSR 0xC0000082 /* LSTAR: Long Mode System Call Target Address */
#define IA32_FMASK_MSR 0xC0000084 /* FMASK: Flags Mask for System Calls */

#define EFER_SCE (1 << 0) /* System Call Extensions flag for EFER */

extern void syscall_entry(void);
extern uintptr_t syscall_kernel_stack_top;

typedef struct __attribute__((packed)) cpu_register_frame {
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
    uint64_t rip;    /* user return RIP */
    uint64_t rflags; /* user return RFLAGS */
} cpu_register_frame_t;

/**
 * Read a Model-Specific Register (MSR).
 */
static inline uint64_t read_msr(uint32_t msr) {
    uint32_t low, high;
    __asm__ volatile("rdmsr" : "=a"(low), "=d"(high) : "c"(msr));
    return ((uint64_t)high << 32) | low;
}

/**
 * Write a Model-Specific Register (MSR).
 */
static inline void write_msr(uint32_t msr, uint64_t value) {
    uint32_t low = (uint32_t)value;
    uint32_t high = (uint32_t)(value >> 32);
    __asm__ volatile("wrmsr" : : "a"(low), "d"(high), "c"(msr));
}

typedef int64_t (*mach_trap_handler_t)(struct cpu_register_frame *frame);

typedef struct mach_trap {
    int64_t trap_number;
    mach_trap_handler_t handler;
} mach_trap_t;

int syscall_init(uintptr_t kernel_stack_top);

/**
 * System call dispatch handler, called from syscall_entry.asm.
 */
uint64_t mach_syscall_dispatch(int64_t syscall_number, struct cpu_register_frame *register_frame);

/* Mach traps. */
int64_t sys_mach_reply_port(struct cpu_register_frame *frame);         /* -26 */
int64_t sys_mach_thread_self_trap(struct cpu_register_frame *frame);   /* -27 */
int64_t sys_task_self_trap(struct cpu_register_frame *frame);          /* -28 */
int64_t sys_host_self_trap(struct cpu_register_frame *frame);          /* -29 */
int64_t sys_mach_msg_trap(struct cpu_register_frame *frame);           /* -31 */
int64_t sys_mach_msg_overwrite_trap(struct cpu_register_frame *frame); /* -32 */

#endif /* CELESTEOS_SYSCALL_H */