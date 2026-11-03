#include "tss.h"

#include <error.h>
#include <ke/log.h>
#include <string.h>

#include <ke/pmm.h>

#define TSS_STACK_SIZE 16384

KeTaskStateSegment g_kernelTss __attribute__((aligned(16)));

/** Bootstrap, double-fault and page-fault stacks for the TSS. */
static uint8_t bootstrap_stack[TSS_STACK_SIZE] __attribute__((aligned(16)));
static uint8_t double_fault_stack[TSS_STACK_SIZE] __attribute__((aligned(16)));
static uint8_t page_fault_stack[TSS_STACK_SIZE] __attribute__((aligned(16)));

void KeTssInitialize(void) {
    memset(&g_kernelTss, 0, sizeof(g_kernelTss));
    g_kernelTss.rsp0 = (uintptr_t)(bootstrap_stack + sizeof(bootstrap_stack));
    g_kernelTss.ist[KE_TSS_IST_DOUBLE_FAULT - 1] =
        (uintptr_t)(double_fault_stack + sizeof(double_fault_stack));
    g_kernelTss.ist[KE_TSS_IST_PAGE_FAULT - 1] =
        (uintptr_t)(page_fault_stack + sizeof(page_fault_stack));
    g_kernelTss.io_map_base = sizeof(KeTaskStateSegment);

    // Prepare TSS for kernel stacks and IST entries.
    KeLog(KE_LOG_INFO, "tss: TSS initialized with bootstrap, #DF, and #PF stacks.\n");
    KeLog(KE_LOG_INFO, "tss: Kernel stack top set to 0x%p.\n", g_kernelTss.rsp0);
    KeLog(KE_LOG_INFO, "tss: #DF stack top set to 0x%p.\n",
        g_kernelTss.ist[KE_TSS_IST_DOUBLE_FAULT - 1]);
    KeLog(KE_LOG_INFO, "tss: #PF stack top set to 0x%p.\n",
        g_kernelTss.ist[KE_TSS_IST_PAGE_FAULT - 1]);
    KeLog(KE_LOG_INFO, "tss: I/O map base set to 0x%p.\n", g_kernelTss.io_map_base);
    KeLog(KE_LOG_INFO, "tss: TSS initialization complete.\n");
    KeTssSetKernelStack((uintptr_t)bootstrap_stack + sizeof(bootstrap_stack));
}

const KeTaskStateSegment *KeTssGet(void) {
    return &g_kernelTss;
}

int KeTssSetKernelStack(uint64_t kernelStackTop) {
    if (kernelStackTop < UINT64_C(0xFFFF800000000000) || (kernelStackTop & 0xF) != 0) {
        return ERR_INVALID_ARG;
    }

    g_kernelTss.rsp0 = kernelStackTop;
    return 0;
}

int KeTssJumpToUserMode(
    uintptr_t entryPoint, uintptr_t userStackTop, KePageTable *pml4, uint64_t hhdmBase) {

    (void)hhdmBase;

    if (!entryPoint || !userStackTop || !pml4) {
        KeLog(KE_LOG_ERROR, "task: Invalid user mode jump arguments!\n");
        return ERR_INVALID_ARG;
    }

    uintptr_t kernelStackTop = (uintptr_t)bootstrap_stack + sizeof(bootstrap_stack);
    KeTssSetKernelStack(kernelStackTop);

    // Switch CR3 to the user page table
    if (KePageMapSwitchTo(pml4) != 0) {
        KeLog(KE_LOG_ERROR, "task: Failed to switch to user page table!\n");
        return ERR_NO_MEMORY;
    }

    KeLog(KE_LOG_INFO, "task: Dropping to ring 3 (rip=0x%p, rsp=0x%p).\n", (void *)entryPoint,
        (void *)userStackTop);

    // Everything is set up for the transition to user mode.
    // Drop privilege level to ring 3 and jump to the user mode entry point!

    KeTssSwitchToRing3(entryPoint, userStackTop);

    __builtin_unreachable();
}