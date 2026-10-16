#ifndef CELESTEOS_TSS_H
#define CELESTEOS_TSS_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "memory/vmm.h"

/**
 * 64-bit Task State Segment (TSS) structure for x86_64 architecture.
 * 
 * On 64 bit, this stores kernel stack pointers for different privilege levels (rings) and
 * interrupt stack table pointers.
 */
typedef struct __attribute__((packed)) tss_t {
    uint32_t reserved0;
    uint64_t rsp0; /* kernel stack pointer */
    uint64_t rsp1; /* ring 1 stack pointer? */
    uint64_t rsp2; /* ring 2 stack pointer? */
    uint64_t reserved1;
    uint64_t ist[7]; /* interrupt stack table pointers */
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t io_map_base;
} tss_t;

extern tss_t kernel_tss;

_Static_assert(sizeof(tss_t) == 104, "64-bit TSS must be 104 bytes");
_Static_assert(offsetof(tss_t, rsp0) == 4, "TSS RSP0 must be at offset 4");
_Static_assert(offsetof(tss_t, ist) == 36, "TSS IST must begin at offset 36");
_Static_assert(offsetof(tss_t, io_map_base) == 102,
               "TSS I/O map base must be at offset 102");

/**
 * Initialize the Task State Segment (TSS) for the kernel, and setup the
 * kernel stack pointers and interrupt stack table (IST) entries for catching #DF and #PF exceptions.
 */
void tss_initialize(void);

/**
 * Prepare the TSS for user mode by setting the kernel stack pointer (RSP0, ring 0).
 */
void tss_prepare_usermode(uintptr_t kernel_stack);

/**
 * Jump to user mode (ring 3) using the TSS and iretq.
 *
 * This function allocates and maps the user stack, sets the kernel stack pointer
 * in the TSS, and then switches to user mode.
 */
bool tss_jump_to_usermode(uintptr_t entry_point, page_table_t* root, uint64_t hhdm_base);

/**
 * Switch from kernel mode to user mode (ring 3) using the TSS and iretq.
 * 
 * This function does the following:
 * - Load the user data segment selector into the data segment registers (DS, ES, FS, GS)
 * - Push the user stack pointer, RFLAGS, code segment selector, and entry point onto the stack
 * - Execute iretq to switch to user mode
 */
extern void tss_switch_ring3(uintptr_t entry_point, uintptr_t user_stack);

#endif  // CELESTEOS_TSS_H
