typedef unsigned int mach_port_t;

/**
 * Emit a Mach syscall trap with no arguments.
 */
static inline mach_port_t mach_trap0(long trap_num) {
    mach_port_t ret;
    __asm__ volatile("movq %1, %%rax\n\t"
                     "syscall\n\t"
                     : "=a"(ret)
                     : "r"(trap_num)
                     : "rcx", "r11", "memory");
    return ret;
}

// Trap -28: sys_task_self_trap
static inline mach_port_t mach_task_self(void) { return mach_trap0(-28); }

// Trap -26: sys_mach_reply_port
static inline mach_port_t mach_reply_port(void) { return mach_trap0(-26); }

// Freestanding Entry Point
void start(void) {
    mach_port_t self = mach_task_self();
    mach_port_t reply = mach_reply_port();

    (void)self;
    (void)reply;

    while (1) {
        __asm__ volatile("pause");
    }
}