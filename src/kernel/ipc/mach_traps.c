#include "mach_traps.h"

#define MACH_PORT_NULL_STUB 0x0
#define TASK_SELF_PORT_HANDLE_STUB 0x1003
#define REPLY_PORT_HANDLE_STUB 0x2003

#define STUB_TRAP(x, dummy_handle)                                                                 \
    do {                                                                                           \
        klog(KLOG_DEBUG, "sys_dispatch: IPC Mach trap: " x " (stub)\n");                           \
        return dummy_handle;                                                                       \
    } while (0)

/**
 * Mach traps (stubs)
 */

mach_port_name_t mach_reply_port(void) {
    STUB_TRAP("mach_reply_port", REPLY_PORT_HANDLE_STUB);
}

mach_port_name_t thread_self_trap(void) {
    STUB_TRAP("thread_self_trap", TASK_SELF_PORT_HANDLE_STUB);
}

mach_port_name_t task_self_trap(void) {
    STUB_TRAP("task_self_trap", TASK_SELF_PORT_HANDLE_STUB);
}

mach_port_name_t host_self_trap(void) {
    STUB_TRAP("host_self_trap", MACH_PORT_NULL_STUB);
}

/**
 * Mach message traps (stubs)
 */

mach_msg_return_t mach_msg_trap(mach_msg_header_t *msg, mach_msg_option_t option,
                                mach_msg_size_t send_size, mach_msg_size_t rcv_size,
                                mach_port_name_t rcv_name, mach_msg_timeout_t timeout,
                                mach_port_name_t notify) {
    STUB_TRAP("mach_msg_trap", KERN_SUCCESS);
}

mach_msg_return_t mach_msg_overwrite_trap(mach_msg_header_t *msg, mach_msg_option_t option,
                                          mach_msg_size_t send_size, mach_msg_size_t rcv_size,
                                          mach_port_name_t rcv_name, mach_msg_timeout_t timeout,
                                          mach_port_name_t notify, mach_msg_header_t *rcv_msg,
                                          mach_msg_size_t rcv_limit) {
    STUB_TRAP("mach_msg_overwrite_trap", KERN_SUCCESS);
}

/**
 * Task and thread traps (stubs)
 */

kern_return_t task_for_pid(mach_port_name_t target_tport, int pid, mach_port_name_t *t) {
    STUB_TRAP("task_for_pid", KERN_SUCCESS);
}

kern_return_t task_name_for_pid(mach_port_name_t target_tport, int pid, mach_port_name_t *tn) {
    STUB_TRAP("task_name_for_pid", KERN_SUCCESS);
}

kern_return_t pid_for_task(mach_port_name_t t, int *pid) {
    STUB_TRAP("pid_for_task", KERN_SUCCESS);
}