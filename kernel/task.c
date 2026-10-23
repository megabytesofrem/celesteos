#include "task.h"

#include <errno.h>
#include <libkernel.h>
#include <string.h>

#include "memory/pmm.h"

tss_t kernel_tss __attribute__((aligned(16)));

static uint8_t kstack[16384] __attribute__((aligned(16)));
static uint8_t df_stack[16384] __attribute__((aligned(16)));
static uint8_t pf_stack[16384] __attribute__((aligned(16)));

/* Base address for the user stack, place it at 0x0000700000000000 */
#define USER_STACK_BASE 0x0000700000000000

/* Number of pages to allocate for the user stack */
#define USER_STACK_PAGES 4

void tss_initialize(void) {
    memset(&kernel_tss, 0, sizeof(kernel_tss));
    kernel_tss.io_map_base = sizeof(tss_t);
    kernel_tss.rsp0 = (uintptr_t)kstack + sizeof(kstack);

    // IST1 for double fault (#DF)
    kernel_tss.ist[0] = (uintptr_t)df_stack + sizeof(df_stack);

    // IST2 for page fault (#PF)
    kernel_tss.ist[1] = (uintptr_t)pf_stack + sizeof(pf_stack);
}

void tss_prepare_usermode(uintptr_t kernel_stack) {
    kernel_tss.rsp0 = kernel_stack;
}

int tss_jump_to_usermode(uintptr_t entry_point, uintptr_t user_stack_top, page_table_t *root,
                         uint64_t hhdm_base) {
    if (!entry_point || !user_stack_top || !root) {
        klog(KLOG_ERROR, "task: Invalid user mode jump arguments!\n");
        return -EINVAL;
    }

    uintptr_t kstack_top = (uintptr_t)kstack + sizeof(kstack);
    tss_prepare_usermode(kstack_top);

    // Switch CR3 to the user page table
    if (pmap_switch_pml4(root) != 0) {
        klog(KLOG_ERROR, "task: Failed to switch to user page table!\n");
        return -ENOMEM;
    }

    klog(KLOG_INFO, "task: Dropping to ring 3 (rip=0x%p, rsp=0x%p).\n", (void *)entry_point,
         (void *)user_stack_top);

    // Everything is set up for the transition to user mode.
    // Drop privilege level to ring 3 and jump to the user mode entry point!

    tss_switch_ring3(entry_point, user_stack_top);

    __builtin_unreachable();
}