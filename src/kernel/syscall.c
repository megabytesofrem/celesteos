#include "syscall.h"

#include <kernel/gdt.h>
#include <libkernel.h>
#include <mach/kern_return.h>

#define MACH_PORT_NULL_STUB 0x0
#define TASK_SELF_PORT_HANDLE_STUB 0x1003
#define REPLY_PORT_HANDLE_STUB 0x2003

#define STUB_TRAP(x, dummy_handle)                                                                 \
    do {                                                                                           \
        klog(KLOG_DEBUG, "kmain: MACH TRAP: " x " (stub)\n");                                      \
        return dummy_handle;                                                                       \
    } while (0)

/**
 * Mapping of Mach trap system call numbers to their handlers.
 */
static const mach_trap_t mach_traps[] = {
    { -26, sys_mach_reply_port }, { -27, sys_mach_thread_self_trap },
    { -28, sys_task_self_trap },  { -29, sys_host_self_trap },
    { -31, sys_mach_msg_trap },   { -32, sys_mach_msg_overwrite_trap },
};

/* Mach traps */
int64_t sys_mach_reply_port(struct cpu_register_frame *frame) {
    // -26: mach_reply_port trap
    STUB_TRAP("sys_mach_reply_port", REPLY_PORT_HANDLE_STUB);
}

int64_t sys_mach_thread_self_trap(struct cpu_register_frame *frame) {
    // -27: mach_thread_self trap
    STUB_TRAP("sys_mach_thread_self_trap", TASK_SELF_PORT_HANDLE_STUB);
}

int64_t sys_task_self_trap(struct cpu_register_frame *frame) {
    // -28: task_self trap
    STUB_TRAP("sys_task_self_trap", TASK_SELF_PORT_HANDLE_STUB);
}

int64_t sys_host_self_trap(struct cpu_register_frame *frame) {
    // -29: host_self trap
    STUB_TRAP("sys_host_self_trap", MACH_PORT_NULL_STUB);
}

int64_t sys_mach_msg_trap(struct cpu_register_frame *frame) {
    // -31: mach_msg trap
    STUB_TRAP("sys_mach_msg_trap", KERN_SUCCESS);
}

int64_t sys_mach_msg_overwrite_trap(struct cpu_register_frame *frame) {
    // -32: mach_msg_overwrite trap
    STUB_TRAP("sys_mach_msg_overwrite_trap", KERN_SUCCESS);
}

/***/

int syscall_init(uintptr_t kernel_stack_top) {
    if (kernel_stack_top == 0) {
        klog(KLOG_ERROR, "kmain: Kernel syscall stack is not initialized.\n");
        return -1;
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

uint64_t mach_syscall_dispatch(int64_t syscall_number, struct cpu_register_frame *frame) {
    // Implement the system call dispatch logic here
    //
    // FIXME: A lot of stubbing. Are we Wine?

    // Log any arguments passed in the register frame
    klog(KLOG_DEBUG,
         "syscall(%d): rax=0x%x rbx=0x%x rdi=0x%x rsi=0x%x rdx=0x%x r8=0x%x r9=0x%x r10=0x%x "
         "r11=0x%x "
         "r12=0x%x "
         "r13=0x%x r14=0x%x r15=0x%x rbp=0x%x rip=0x%x rflags=0x%x\n",
         syscall_number, frame->rax, frame->rbx, frame->rdi, frame->rsi, frame->rdx, frame->r8,
         frame->r9, frame->r10, frame->r11, frame->r12, frame->r13, frame->r14, frame->r15,
         frame->rbp, frame->rip, frame->rflags);

    size_t n_traps = sizeof(mach_traps) / sizeof(mach_traps[0]);
    for (size_t i = 0; i < n_traps; i++) {
        if (mach_traps[i].trap_number == syscall_number) {
            return mach_traps[i].handler(frame);
        }
    }

    if (syscall_number < 0) {
        // Mach traps start from negative syscall numbers
        klog(KLOG_WARN, "kmain: Unhandled Mach trap: %d\n, stub needed\n", syscall_number);
    } else {
        // BSD syscalls are non-negative syscall numbers starting at 0x2000001
        klog(KLOG_WARN, "kmain: Unhandled BSD syscall: 0x%x, stub needed\n", syscall_number);
    }

    return -1;
}