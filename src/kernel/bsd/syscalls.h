#ifndef CELESTEOS_BSD_SYSCALLS_H
#define CELESTEOS_BSD_SYSCALLS_H

#include <libkernel.h>

#define MACH_BSD_OFFSET 0x200000

#define STUB_SYS(x)                                                                                \
    do {                                                                                           \
        klog(KLOG_DEBUG, "kmain/bsd: BSD CALL: " x " (stub)\n");                                   \
    } while (0)

#define SYS_EXIT 1  /* sys_exit(int status) */
#define SYS_FORK 2  /* sys_fork(void) */
#define SYS_READ 3  /* sys_read(int fd, void *buf, size_t count) */
#define SYS_WRITE 4 /* sys_write(int fd, const void *buf, size_t count) */
#define SYS_OPEN 5  /* sys_open(const char *pathname, int flags, mode_t mode) */
#define SYS_CLOSE 6 /* sys_close */

/**
 * All BSD system calls are offset by MACH_BSD_OFFSET (0x200000) following the XNU kernel
 * convention.
 */
#define BSD(x) (MACH_BSD_OFFSET + (x))

#endif /* CELESTEOS_BSD_SYSCALLS_H */