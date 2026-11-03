#ifndef CELESTEOS_TSS_H
#define CELESTEOS_TSS_H

#include <stddef.h>
#include <stdint.h>

#include <ke/page.h>

enum {
    KE_TSS_IST_DOUBLE_FAULT = 1,
    KE_TSS_IST_PAGE_FAULT = 2
};

typedef struct __attribute__((packed)) KeTaskStateSegment {
    uint32_t reserved0;
    uint64_t rsp0;
    uint64_t rsp1;
    uint64_t rsp2;
    uint64_t reserved1;
    uint64_t ist[7];
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t io_map_base;
} KeTaskStateSegment;

extern KeTaskStateSegment g_kernelTss;

/**
 * Initialize the boot CPU TSS and dedicated bootstrap, double-fault and
 * page-fault stacks. Called once by gdt_init before loading the task register.
 */
void KeTssInitialize(void);

/**
 * Return the bootstrap CPU TSS for installation in the GDT.
 */
const KeTaskStateSegment *KeTssGet(void);

/**
 * Prepare the TSS for use by a user thread by setting RSP0 to the given
 * kernel stack top. The stack must remain mapped in every address space
 * using this TSS.
 */
int KeTssSetKernelStack(uint64_t kernelStackTop);

/**
 * Jump to user mode (ring 3) using the provided entry point, user stack, and page table.
 *
 * This function maps the user stack, sets up the kernel stack pointer (RSP0) in the TSS
 * and performs the transition to user mode using an iretq instruction.
 */
int KeTssJumpToUserMode(
    uintptr_t entryPoint, uintptr_t userStackTop, KePageTable *pml4, uint64_t hhdmBase);

extern void KeTssSwitchToRing3(uintptr_t entryPoint, uintptr_t userStackTop);

#endif /* CELESTEOS_TSS_H */