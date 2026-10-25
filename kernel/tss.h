#ifndef CELESTEOS_TSS_H
#define CELESTEOS_TSS_H

#include <stddef.h>
#include <stdint.h>

#include <kernel/pmap.h>

enum {
    TSS_IST_DOUBLE_FAULT = 1,
    TSS_IST_PAGE_FAULT = 2
};

typedef struct __attribute__((packed)) task_state_segment {
    uint32_t reserved0;
    uint64_t rsp0;
    uint64_t rsp1;
    uint64_t rsp2;
    uint64_t reserved1;
    uint64_t ist[7];
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t io_map_base;
} tss_t;

extern tss_t k_tss;

/**
 * Initialize the boot CPU TSS and dedicated bootstrap, double-fault and
 * page-fault stacks. Called once by gdt_init before loading the task register.
 */
void tss_init(void);

/**
 * Return the bootstrap CPU TSS for installation in the GDT.
 */
const tss_t *tss_get(void);

/**
 * Prepare the TSS for use by a user thread by setting RSP0 to the given
 * kernel stack top. The stack must remain mapped in every address space
 * using this TSS.
 */
int tss_prepare_user(uint64_t kernel_stack_top);

/**
 * Jump to user mode (ring 3) using the provided entry point, user stack, and page table.
 *
 * This function maps the user stack, sets up the kernel stack pointer (RSP0) in the TSS
 * and performs the transition to user mode using an iretq instruction.
 */
int tss_jump_to_usermode(uintptr_t entry_point, uintptr_t user_stack_top, page_table_t *pml4,
                         uint64_t hhdm_base);

extern void tss_switch_ring3(uintptr_t entry_point, uintptr_t user_stack_top);

#endif /* CELESTEOS_TSS_H */