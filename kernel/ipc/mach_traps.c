#include "mach_traps.h"

#include <ipc/ipc_space.h>
#include <ipc/task.h>
#include <ipc/thread.h>
#include <kernel/memory/pmap.h>
#include <kernel/stub.h>

static kern_return_t create_receive_port(mach_port_name_t *name) {
    task_t *task = current_task();
    if (task == NULL || task->itk_space == NULL || name == NULL) {
        return KERN_FAILURE;
    }
    ipc_port_t *port;
    kern_return_t result = ipc_port_alloc(task->itk_space, 0, name, &port);
    if (result == KERN_SUCCESS) {
        ipc_port_release(port);
    }
    return result;
}

mach_port_name_t mach_reply_port(void) {
    mach_port_name_t name = MACH_PORT_NULL;

    // Attempt to create a receive port for the reply
    if (create_receive_port(&name) != KERN_SUCCESS) {
        return MACH_PORT_NULL;
    }
    return name;
}

mach_port_name_t thread_self_trap(void) {
    // Grab the current thread and return its self port (itk_self) if available
    thread_t *thread = current_thread();
    return thread != NULL ? thread->itk_self : MACH_PORT_NULL;
}

mach_port_name_t task_self_trap(void) {
    // Grab the current task and return its self port (itk_self) if available
    task_t *task = current_task();
    return task != NULL ? task->itk_self : MACH_PORT_NULL;
}

mach_port_name_t host_self_trap(void) {
    UNIMPLEMENTED;
    return MACH_PORT_NULL;
}

mach_msg_return_t mach_msg_trap(mach_msg_header_t *msg, mach_msg_option_t option,
                                mach_msg_size_t send_size, mach_msg_size_t rcv_size,
                                mach_port_name_t rcv_name, mach_msg_timeout_t timeout,
                                mach_port_name_t notify) {
    mach_msg_return_t result =
        mach_msg(msg, option, send_size, rcv_size, rcv_name, timeout, notify);

    // Just klog the result of mach_msg
    int log_type = result == MACH_MSG_SUCCESS ? KLOG_DEBUG : KLOG_WARN;

    klog(log_type, "ipc: mach_msg returned 0x%x (%s)\n", (unsigned int)result,
         result == MACH_MSG_SUCCESS ? "MACH_MSG_SUCCESS" : "MACH_MSG_FAILURE");
    return result;
}

mach_msg_return_t mach_msg_overwrite_trap(mach_msg_header_t *msg, mach_msg_option_t option,
                                          mach_msg_size_t send_size, mach_msg_size_t rcv_size,
                                          mach_port_name_t rcv_name, mach_msg_timeout_t timeout,
                                          mach_port_name_t notify, mach_msg_header_t *rcv_msg,
                                          mach_msg_size_t rcv_limit) {
    (void)notify;

    // Validate the options provided to the mach_msg_overwrite call
    const mach_msg_option_t supported = MACH_SEND_MSG | MACH_RCV_MSG | MACH_RCV_TIMEOUT;
    if ((option & ~supported) != 0 || (option & (MACH_SEND_MSG | MACH_RCV_MSG)) == 0 ||
        ((option & MACH_RCV_TIMEOUT) != 0 && (option & MACH_RCV_MSG) == 0)) {
        return MACH_SEND_INVALID_DATA;
    }
    if ((option & MACH_SEND_MSG) != 0) {
        mach_msg_return_t result = mach_msg_send(msg, option, send_size, timeout);
        if (result != MACH_MSG_SUCCESS) {
            return result;
        }
    }
    if ((option & MACH_RCV_MSG) != 0) {
        mach_msg_header_t *receive_buffer = rcv_msg != NULL ? rcv_msg : msg;
        mach_msg_size_t receive_size = rcv_limit != 0 ? rcv_limit : rcv_size;
        return mach_msg_receive(receive_buffer, option, receive_size, rcv_name, timeout);
    }

    // If we reach here, it means neither MACH_SEND_MSG nor MACH_RCV_MSG was specified, which is
    // invalid behavior
    return (option & ~(MACH_SEND_MSG | MACH_RCV_MSG)) == 0 ? MACH_MSG_SUCCESS
                                                           : MACH_SEND_INVALID_DATA;
}

kern_return_t task_for_pid(mach_port_name_t target_tport, int pid, mach_port_name_t *t) {
    (void)target_tport;
    (void)pid;
    (void)t;

    UNIMPLEMENTED;
    return KERN_SUCCESS;
}

kern_return_t task_name_for_pid(mach_port_name_t target_tport, int pid, mach_port_name_t *tn) {
    (void)target_tport;
    (void)pid;
    (void)tn;

    UNIMPLEMENTED;
    return KERN_SUCCESS;
}

kern_return_t pid_for_task(mach_port_name_t t, int *pid) {
    (void)t;
    (void)pid;

    UNIMPLEMENTED;
    return KERN_SUCCESS;
}

kern_return_t mach_port_allocate_trap(mach_port_right_t right, mach_port_name_t *name) {
    if (right != MACH_PORT_RIGHT_RECEIVE || name == NULL) {
        return KERN_INVALID_ARGUMENT;
    }
    mach_port_name_t port_name;

    // Attempt to create a new receive port
    kern_return_t result = create_receive_port(&port_name);
    if (result != KERN_SUCCESS) {
        klog(KLOG_WARN, "ipc: receive-port allocation failed (%u)\n", (unsigned int)result);
        return result;
    }

    // Copy the allocated port name to user space
    if (pmap_copy_to_user((uintptr_t)name, &port_name, sizeof(port_name)) != 0) {
        (void)ipc_space_remove(current_task()->itk_space, port_name);
        return KERN_INVALID_ARGUMENT;
    }

    // Allocated a new receive right successfully
    klog(KLOG_DEBUG, "ipc: allocated receive right %u\n", (unsigned int)port_name);
    return KERN_SUCCESS;
}

kern_return_t mach_port_make_send_trap(mach_port_name_t receive_name, mach_port_name_t *send_name) {
    task_t *task = current_task();
    if (task == NULL || task->itk_space == NULL || send_name == NULL) {
        return KERN_INVALID_ARGUMENT;
    }
    *send_name = MACH_PORT_NULL;

    ipc_object_t *object;
    mach_port_right_t right;
    kern_return_t result = ipc_space_lookup(task->itk_space, receive_name, &object, &right);
    if (result != KERN_SUCCESS) {
        return result;
    }
    if (right != MACH_PORT_RIGHT_RECEIVE || io_otype(object) != IO_TYPE_PORT ||
        !io_active(object)) {
        ipc_object_release(object);
        return KERN_PERMISSION_DENIED;
    }

    mach_port_name_t name;
    result = ipc_space_insert_anon(task->itk_space, object, MACH_PORT_RIGHT_SEND, &name);
    ipc_object_release(object);
    if (result != KERN_SUCCESS) {
        return result;
    }
    if (pmap_copy_to_user((uintptr_t)send_name, &name, sizeof(name)) != 0) {
        (void)ipc_space_remove(task->itk_space, name);
        return KERN_INVALID_ARGUMENT;
    }
    klog(KLOG_DEBUG, "ipc: made send right %u for receive right %u\n", (unsigned int)name,
         (unsigned int)receive_name);
    return KERN_SUCCESS;
}

kern_return_t mach_port_deallocate_trap(mach_port_name_t name) {
    task_t *task = current_task();
    if (task == NULL || task->itk_space == NULL) {
        return KERN_FAILURE;
    }
    return ipc_space_remove(task->itk_space, name);
}