#include "syscall.h"

#include <kernel/gdt.h>
#include <libkernel.h>

#define STUB_TRAP(x, dummy_handle)                                                                 \
    do {                                                                                           \
        klog(KLOG_WARN, "STUB TRAP: " x "\n");                                                     \
        return dummy_handle;                                                                       \
    } while (0)

int syscall_init(uintptr_t kernel_stack_top) {
    if (kernel_stack_top == 0) {
        klog(KLOG_ERROR, "syscall: Kernel stack is not initialized.\n");
        return -1;
    }

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

/* Mach traps */
int64_t sys_mach_reply_port(void) {
    // Implement the mach reply port trap here
    STUB_TRAP("sys_mach_reply_port", 0x1003);
}

int64_t sys_mach_msg_trap(void) {
    // Implement the mach message trap here
    STUB_TRAP("sys_mach_msg_trap", 0x1006);
}

int64_t sys_mach_msg_overwrite_trap(void) {
    // Implement the mach message overwrite trap here
    STUB_TRAP("sys_mach_msg_overwrite_trap", 0x1006);
}

int64_t sys_task_self_trap(void) {
    // Implement the task self trap here
    STUB_TRAP("sys_task_self_trap", 0x1004);
}

int64_t sys_host_self_trap(void) {
    // Implement the host self trap here
    STUB_TRAP("sys_host_self_trap", 0x1005);
}

uint64_t mach_syscall_dispatch(uint64_t syscall_number, struct cpu_register_frame *register_frame) {
    // Implement the system call dispatch logic here
    //
    // FIXME: A lot of stubbing. Are we Wine?

    // Log any arguments passed in the register frame
    klog(KLOG_DEBUG,
         "syscall(%d): rax=0x%x rbx=0x%x rdi=0x%x rsi=0x%x rdx=0x%x r8=0x%x r9=0x%x r10=0x%x "
         "r11=0x%x "
         "r12=0x%x "
         "r13=0x%x r14=0x%x r15=0x%x rbp=0x%x rip=0x%x rflags=0x%x\n",
         syscall_number, register_frame->rax, register_frame->rbx, register_frame->rdi,
         register_frame->rsi, register_frame->rdx, register_frame->r8, register_frame->r9,
         register_frame->r10, register_frame->r11, register_frame->r12, register_frame->r13,
         register_frame->r14, register_frame->r15, register_frame->rbp, register_frame->rip,
         register_frame->rflags);

    switch (syscall_number) {
    case -26:
        return sys_mach_reply_port(); /* _mach_reply_port */
    case -27:
        return sys_task_self_trap(); /* _task_self_trap */
    case -28:
        return sys_host_self_trap(); /* _host_self_trap */
    case -31:
        return sys_mach_msg_trap(); /* _mach_msg_trap */
    case -32:
        return sys_mach_msg_overwrite_trap(); /* _mach_msg_overwrite_trap */
    default:
        if ((int64_t)syscall_number < 0) {
            klog(KLOG_WARN, "Unhandled Mach trap: %d", syscall_number);
            return -1;
        }

        klog(KLOG_WARN, "Unknown syscall number: %d", syscall_number);
        return -1;
    }
}