typedef int kern_return_t;
typedef unsigned int mach_port_t;
typedef unsigned int mach_port_name_t;
typedef unsigned int mach_port_right_t;

#define KERN_SUCCESS 0

/* Simulated Mach Trap / System Call Numbers for XNU */
#define SYS_mach_msg_trap (-31)
#define SYS_task_self_trap (-28)
#define SYS_host_self_trap (-29)
#define SYS_mach_reply_port_trap (-26)

/* Bare-metal inline assembly for 64-bit Mach syscalls */
static inline kern_return_t mach_syscall_0(int num) {
    long ret;
    __asm__ volatile("movq %1, %%rax\n\t"
                     "syscall\n\t"
                     : "=a"(ret)
                     : "r"((long)num)
                     : "rcx", "r11", "memory");
    return (kern_return_t)ret;
}

static inline mach_port_t mach_task_self(void) {
    return (mach_port_t)mach_syscall_0(SYS_task_self_trap);
}

static inline mach_port_t mach_reply_port(void) {
    return (mach_port_t)mach_syscall_0(SYS_mach_reply_port_trap);
}

/* Explicit Entry Point */
void _start(void) {
    /* 1. Request task_self handle */
    mach_port_t self_task = mach_task_self();

    /* 2. Request a Mach reply port */
    mach_port_t reply_port = mach_reply_port();

    /* 3. Dummy spin loop to prevent falling off the edge */
    while (1) {
        __asm__ volatile("pause");
    }
}