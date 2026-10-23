#include "syscall.h"

#include <kernel/gdt.h>
#include <kernel/sys_dispatch.h>

#include <errno.h>
#include <libkernel.h>

int syscall_init(uintptr_t kernel_stack_top) {
    if (kernel_stack_top == 0) {
        klog(KLOG_ERROR, "kmain: Kernel syscall stack is not initialized.\n");
        return -EINVAL;
    }

    // Initialize the kernel stack top for system calls
    syscall_kernel_stack_top = kernel_stack_top;

    // Enable System Call Extensions (SCE) in the EFER MSR
    uint64_t efer = read_msr(IA32_EFER_MSR);
    write_msr(IA32_EFER_MSR, efer | EFER_SCE);

    // Set kernel target address for system calls in LSTAR MSR
    write_msr(IA32_LSTAR_MSR, (uint64_t)syscall_entry);

    // Prepare the STAR MSR value with kernel and user code segment selectors
    uint64_t star =
        ((uint64_t)GDT_OFFSET_KERNEL_CODE << 32) | ((uint64_t)(GDT_OFFSET_USER_CODE - 8) << 48);

    // Clear interrupt flag mask in FMASK MSR
    write_msr(IA32_FMASK_MSR, 0x200); /* Clear the interrupt flag (IF) in FMASK MSR */

    // Write the STAR MSR with the kernel and user CS selectors
    write_msr(IA32_STAR_MSR, star);
    return 0;
}

uint64_t mach_syscall_dispatch(int64_t syscall_number, stack_frame_t *frame) {
    return sys_dispatch(syscall_number, frame);
}