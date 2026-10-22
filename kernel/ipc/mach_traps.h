#ifndef CELESTEOS_MACH_TRAPS_H
#define CELESTEOS_MACH_TRAPS_H

#include <stddef.h>

#include <ipc/ipc_port.h>
#include <ipc/mach_message.h>
#include <mach/mach_types.h>

/**
 * General Mach traps.
 */

mach_port_name_t mach_reply_port(void);
mach_port_name_t thread_self_trap(void);
mach_port_name_t task_self_trap(void);
mach_port_name_t host_self_trap(void);

mach_msg_return_t mach_msg_trap(mach_msg_header_t *msg, mach_msg_option_t option,
                                mach_msg_size_t send_size, mach_msg_size_t rcv_size,
                                mach_port_name_t rcv_name, mach_msg_timeout_t timeout,
                                mach_port_name_t notify);

mach_msg_return_t mach_msg_overwrite_trap(mach_msg_header_t *msg, mach_msg_option_t option,
                                          mach_msg_size_t send_size, mach_msg_size_t rcv_size,
                                          mach_port_name_t rcv_name, mach_msg_timeout_t timeout,
                                          mach_port_name_t notify, mach_msg_header_t *rcv_msg,
                                          mach_msg_size_t rcv_limit);

/**
 * Task and thread traps
 */

kern_return_t task_for_pid(mach_port_name_t target_tport, int pid, mach_port_name_t *t);
kern_return_t task_name_for_pid(mach_port_name_t target_tport, int pid, mach_port_name_t *tn);
kern_return_t pid_for_task(mach_port_name_t t, int *pid);

/**
 * Local port-management calls exposed as kernel traps by celesteOS.
 */
kern_return_t mach_port_allocate_trap(mach_port_right_t right, mach_port_name_t *name);
kern_return_t mach_port_make_send_trap(mach_port_name_t receive_name,
                                       mach_port_name_t *send_name);
kern_return_t mach_port_deallocate_trap(mach_port_name_t name);

#endif // CELESTEOS_MACH_TRAPS_H