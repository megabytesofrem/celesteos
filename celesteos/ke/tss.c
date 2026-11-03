#include "tss.h"

#include <error.h>
#include <ke/log.h>
#include <string.h>

#include <ke/pmm.h>

#define TSS_STACK_SIZE 16384

KeTaskStateSegment gKernelTss __attribute__((aligned(16)));

/** Bootstrap, double-fault and page-fault stacks for the TSS. */
static uint8_t bootstrapStack[TSS_STACK_SIZE] __attribute__((aligned(16)));
static uint8_t doubleFaultStack[TSS_STACK_SIZE] __attribute__((aligned(16)));
static uint8_t pageFaultStack[TSS_STACK_SIZE] __attribute__((aligned(16)));

void KeTssInitialize(void) {
    memset(&gKernelTss, 0, sizeof(gKernelTss));
    gKernelTss.rsp0 = (uintptr_t)(bootstrapStack + sizeof(bootstrapStack));
    gKernelTss.ist[KE_TSS_IST_DOUBLE_FAULT - 1] =
        (uintptr_t)(doubleFaultStack + sizeof(doubleFaultStack));
    gKernelTss.ist[KE_TSS_IST_PAGE_FAULT - 1] =
        (uintptr_t)(pageFaultStack + sizeof(pageFaultStack));
    gKernelTss.ioMapBase = sizeof(KeTaskStateSegment);

    // Prepare TSS for kernel stacks and IST entries.
    KeLog(KE_LOG_INFO, "tss: TSS initialized with bootstrap, #DF, and #PF stacks.\n");
    KeLog(KE_LOG_INFO, "tss: Kernel stack top set to 0x%p.\n", gKernelTss.rsp0);
    KeLog(KE_LOG_INFO, "tss: #DF stack top set to 0x%p.\n",
        gKernelTss.ist[KE_TSS_IST_DOUBLE_FAULT - 1]);
    KeLog(KE_LOG_INFO, "tss: #PF stack top set to 0x%p.\n",
        gKernelTss.ist[KE_TSS_IST_PAGE_FAULT - 1]);
    KeLog(KE_LOG_INFO, "tss: I/O map base set to 0x%p.\n", gKernelTss.ioMapBase);
    KeLog(KE_LOG_INFO, "tss: TSS initialization complete.\n");
    KeTssSetKernelStack((uintptr_t)bootstrapStack + sizeof(bootstrapStack));
}

const KeTaskStateSegment *KeTssGet(void) {
    return &gKernelTss;
}

int KeTssSetKernelStack(uint64_t kernelStackTop) {
    if (kernelStackTop < UINT64_C(0xFFFF800000000000) || (kernelStackTop & 0xF) != 0) {
        return ERR_INVALID_ARG;
    }

    gKernelTss.rsp0 = kernelStackTop;
    return 0;
}

int KeTssJumpToUserMode(
    uintptr_t entryPoint, uintptr_t userStackTop, KePageTable *pml4, uint64_t hhdmBase) {

    (void)hhdmBase;

    if (!entryPoint || !userStackTop || !pml4) {
        KeLog(KE_LOG_ERROR, "task: Invalid user mode jump arguments!\n");
        return ERR_INVALID_ARG;
    }

    uintptr_t kernelStackTop = (uintptr_t)bootstrapStack + sizeof(bootstrapStack);
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