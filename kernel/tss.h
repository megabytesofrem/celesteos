#ifndef CELESTEOS_KERNEL_TSS_H
#define CELESTEOS_KERNEL_TSS_H

#include <stdint.h>

typedef struct task_state_segment {
    uint32_t reserved0;
    uint32_t rsp0;
    uint32_t rsp1;
    uint32_t rsp2;
    uint64_t reserved1;
    uint64_t ist[7];
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t io_map_base;
} tss_t;

extern tss_t KERNEL_TSS;

/**
 * Initialize the Task State Segment (TSS) for the kernel, and setup
 * the stack pointers and interrupt stack table (IST) entries.
 */
void tss_init(void);

/**
 * Prepare the Task State Segment (TSS) for user mode, setting up
 * the appropriate stack pointers and interrupt stack table (IST) entries.
 */
void tss_prepare_user(void);

#endif /* CELESTEOS_KERNEL_TSS_H */