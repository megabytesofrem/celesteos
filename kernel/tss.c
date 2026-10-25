#include "tss.h"

#include <errno.h>
#include <klog.h>
#include <string.h>

#include <kernel/pmm.h>

#define TSS_STACK_SIZE 16384

tss_t k_tss __attribute__((aligned(16)));

/** Bootstrap, double-fault and page-fault stacks for the TSS. */
static uint8_t bootstrap_stack[TSS_STACK_SIZE] __attribute__((aligned(16)));
static uint8_t double_fault_stack[TSS_STACK_SIZE] __attribute__((aligned(16)));
static uint8_t page_fault_stack[TSS_STACK_SIZE] __attribute__((aligned(16)));

void tss_init(void) {
    memset(&k_tss, 0, sizeof(k_tss));
    k_tss.rsp0 = (uintptr_t)(bootstrap_stack + sizeof(bootstrap_stack));
    k_tss.ist[TSS_IST_DOUBLE_FAULT - 1] =
        (uintptr_t)(double_fault_stack + sizeof(double_fault_stack));
    k_tss.ist[TSS_IST_PAGE_FAULT - 1] = (uintptr_t)(page_fault_stack + sizeof(page_fault_stack));
    k_tss.io_map_base = sizeof(tss_t);

    // Prepare TSS for kernel stacks and IST entries.
    klog(KLOG_INFO, "tss: TSS initialized with bootstrap, #DF, and #PF stacks.\n");
    klog(KLOG_INFO, "tss: Kernel stack top set to 0x%p.\n", k_tss.rsp0);
    klog(KLOG_INFO, "tss: #DF stack top set to 0x%p.\n", k_tss.ist[TSS_IST_DOUBLE_FAULT - 1]);
    klog(KLOG_INFO, "tss: #PF stack top set to 0x%p.\n", k_tss.ist[TSS_IST_PAGE_FAULT - 1]);
    klog(KLOG_INFO, "tss: I/O map base set to 0x%p.\n", k_tss.io_map_base);
    klog(KLOG_INFO, "tss: TSS initialization complete.\n");
    tss_prepare_user((uintptr_t)bootstrap_stack + sizeof(bootstrap_stack));
}

const tss_t *tss_get(void) {
    return &k_tss;
}

int tss_prepare_user(uint64_t kernel_stack_top) {
    if (kernel_stack_top < UINT64_C(0xFFFF800000000000) || (kernel_stack_top & 0xF) != 0) {
        return -EINVAL;
    }

    k_tss.rsp0 = kernel_stack_top;
    return 0;
}

int tss_jump_to_usermode(uintptr_t entry_point, uintptr_t user_stack_top, page_table_t *pml4,
                         uint64_t hhdm_base) {

    (void)hhdm_base;

    if (!entry_point || !user_stack_top || !pml4) {
        klog(KLOG_ERROR, "task: Invalid user mode jump arguments!\n");
        return -EINVAL;
    }

    uintptr_t kstack_top = (uintptr_t)bootstrap_stack + sizeof(bootstrap_stack);
    tss_prepare_user(kstack_top);

    // Switch CR3 to the user page table
    if (pmap_switch_pml4(pml4) != 0) {
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