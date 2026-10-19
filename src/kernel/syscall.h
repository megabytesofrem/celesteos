#ifndef CELESTEOS_SYSCALL_H
#define CELESTEOS_SYSCALL_H

#include <kernel/stack_frame.h>
#include <stdint.h>

#define IA32_EFER_MSR 0xC0000080  /* EFER: Extended Feature Enable Register */
#define IA32_STAR_MSR 0xC0000081  /* STAR: Segment Target Address Register */
#define IA32_LSTAR_MSR 0xC0000082 /* LSTAR: Long Mode System Call Target Address */
#define IA32_FMASK_MSR 0xC0000084 /* FMASK: Flags Mask for System Calls */

#define EFER_SCE (1 << 0) /* System Call Extensions flag for EFER */

extern void syscall_entry(void);
extern uintptr_t syscall_kernel_stack_top;

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

int syscall_init(uintptr_t kernel_stack_top);

/**
 * System call dispatch handler, called from syscall_entry.asm.
 */
uint64_t mach_syscall_dispatch(int64_t syscall_number, stack_frame_t *register_frame);

#endif /* CELESTEOS_SYSCALL_H */