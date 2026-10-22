/**
 * OS X offsets its BSD syscalls by 0x2000000
 */
#define BSD_SYSCALL_BASE 0x2000000

#define SYS_WRITE (BSD_SYSCALL_BASE + 4) /* sys_write */
#define SYS_EXIT (BSD_SYSCALL_BASE + 1)  /* sys_exit */

/**
 * Emit a BSD syscall (sys_write)
 */
static inline long sys_write(int fd, const void *buf, unsigned long count) {
    long ret;

    // clang-format off

    /**
     * rax: syscall number (0x2000004 for sys_write)
     * rdi: file descriptor (fd)
     * rsi: buffer pointer (buf)
     * rdx: byte count (count)
     */
    __asm__ volatile(
        "syscall"
        : "=a"(ret)                                            /* output: rax */
        : "a"(SYS_WRITE), "D"((long)fd), "S"(buf), "d"(count)  /* inputs: rax, rdi, rsi, rdx */
        : "rcx", "r11", "memory"                               /* clobbers: rcx, r11, memory */
    );
    // clang-format on
    return ret;
}

/**
 * Emit a BSD syscall (sys_exit)
 */
static inline void sys_exit(int status) {
    /**
     * rax: syscall number (0x2000001 for sys_exit)
     * rdi: exit status (status)
     *
     * inputs: rax (0x2000001), rdi (status)
     * clobbers: rcx, r11, memory
     */
    __asm__ volatile("syscall" : : "a"(SYS_EXIT), "D"((long)status) : "rcx", "r11", "memory");
}

void start(void) {
    const char msg[] = "hello";
    sys_write(1, msg, sizeof(msg) - 1);

    sys_exit(0);

    while (1) {
        __asm__ volatile("pause");
    }
}
