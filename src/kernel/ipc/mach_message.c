#include "mach_message.h"

#include <ipc/ipc_port.h>
#include <ipc/task.h>
#include <ipc/thread.h>
#include <kernel/kmalloc.h>
#include <kernel/memory/pmap.h>

#define MACH_MSG_MAX_SIZE (64u * 1024u)

static void requeue_message_front(ipc_port_t *port, mach_kmsg_t *message) {
    ip_lock(port);
    message->prev = NULL;
    message->next = port->msg_queue_head;
    if (port->msg_queue_head != NULL) {
        port->msg_queue_head->prev = message;
    } else {
        port->msg_queue_tail = message;
    }
    port->msg_queue_head = message;
    port->msg_count++;
    ip_unlock(port);
}

static ipc_port_t *lookup_port(mach_port_name_t name, mach_port_right_t required_right,
                               mach_msg_return_t *error) {

    // Grab the current task and its IPC space (task->itk_space)
    task_t *task = current_task();
    if (task == NULL || task->itk_space == NULL) {
        *error = required_right == MACH_PORT_RIGHT_RECEIVE ? MACH_RCV_INVALID_NAME
                                                           : MACH_SEND_INVALID_DEST;
        return NULL;
    }

    ipc_object_t *object = NULL;
    mach_port_right_t right;

    // Look up the IPC object corresponding to the given port name in the task's IPC space
    if (ipc_space_lookup(task->itk_space, name, &object, &right) != KERN_SUCCESS) {
        *error = required_right == MACH_PORT_RIGHT_RECEIVE ? MACH_RCV_INVALID_NAME
                                                           : MACH_SEND_INVALID_DEST;
        return NULL;
    }
    // If the right does not match, or the object is not active, or the object is not a port, return
    // NULL and set the error out pointer
    if (right != required_right || !io_active(object) || io_otype(object) != IO_TYPE_PORT) {
        ipc_object_release(object);
        *error = required_right == MACH_PORT_RIGHT_RECEIVE ? MACH_RCV_INVALID_NAME
                                                           : MACH_SEND_INVALID_RIGHT;
        return NULL;
    }

    // Got our port, cast and return!
    return (ipc_port_t *)object;
}

mach_msg_return_t mach_msg(mach_msg_header_t *msg, mach_msg_option_t option,
                           mach_msg_size_t send_size, mach_msg_size_t rcv_size,
                           mach_port_name_t rcv_name, mach_msg_timeout_t timeout,
                           mach_port_name_t notify) {
    (void)notify;
    const mach_msg_option_t supported = MACH_SEND_MSG | MACH_RCV_MSG | MACH_RCV_TIMEOUT;

    // Validate the options provided to the mach_msg call
    if ((option & ~supported) != 0 || (option & (MACH_SEND_MSG | MACH_RCV_MSG)) == 0) {
        return MACH_SEND_INVALID_DATA;
    }
    if ((option & MACH_RCV_TIMEOUT) != 0 && (option & MACH_RCV_MSG) == 0) {
        return MACH_SEND_INVALID_DATA;
    }

    /**
     * Check the option flag and determine what we need to do:
     *
     * - MACH_SEND_MSG: we need to dispatch to mach_msg_send to send a message
     * - MACH_RCV_MSG:  we need to dispatch to mach_msg_receive to receive a message
     */
    if ((option & MACH_SEND_MSG) != 0) {
        mach_msg_return_t result = mach_msg_send(msg, option, send_size, timeout);
        if (result != MACH_MSG_SUCCESS) {
            return result;
        }
    }
    if ((option & MACH_RCV_MSG) != 0) {
        return mach_msg_receive(msg, option, rcv_size, rcv_name, timeout);
    }
    return MACH_MSG_SUCCESS;
}

mach_msg_return_t mach_msg_send(mach_msg_header_t *msg, mach_msg_option_t option,
                                mach_msg_size_t send_size, mach_msg_timeout_t timeout) {

    FIXME("does not handle option and timeout at all");

    (void)option;
    (void)timeout;

    // Validate the message pointer and size
    if (msg == NULL || send_size < sizeof(mach_msg_header_t) || send_size > MACH_MSG_MAX_SIZE) {
        return MACH_SEND_INVALID_DATA;
    }

    // Copy the message header from user space and validate it
    mach_msg_header_t header;
    if (pmap_copy_from_user(&header, (uintptr_t)msg, sizeof(header)) != 0 ||
        header.msgh_size < sizeof(header) || header.msgh_size > send_size ||
        header.msgh_size > MACH_MSG_MAX_SIZE || (header.msgh_bits & MACH_MSGH_BITS_COMPLEX) != 0) {
        return MACH_SEND_INVALID_DATA;
    }

    if (header.msgh_bits & MACH_MSGH_BITS_COMPLEX) {
        FIXME("unhandled MACH_MSGH_BITS_COMPLEX");
        return MACH_SEND_INVALID_DATA;
    }

    // Look up the send right for the remote port specified in the message header
    mach_msg_return_t error;
    ipc_port_t *port = lookup_port(header.msgh_remote_port, MACH_PORT_RIGHT_SEND, &error);
    if (port == NULL) {
        return error;
    }

    // Allocate a kernel message structure to hold the message data
    size_t allocation_size = sizeof(mach_kmsg_t) + header.msgh_size;
    mach_kmsg_t *message = kmalloc(allocation_size);
    if (message == NULL) {
        ipc_port_release(port);
        return MACH_SEND_NO_BUFFER;
    }

    // Copy the message data from user space into the kernel message structure
    message->header = (mach_msg_header_t *)message->data;
    if (pmap_copy_from_user(message->data, (uintptr_t)msg, header.msgh_size) != 0) {
        kfree(message);
        ipc_port_release(port);
        return MACH_SEND_INVALID_DATA;
    }

    // Enqueue the message onto the port's message queue
    kern_return_t result = ipc_port_enqueue_kmsg(port, message);
    ipc_port_release(port);
    if (result == KERN_NO_MEMORY) {
        kfree(message);
        return MACH_SEND_NO_BUFFER;
    }
    if (result != KERN_SUCCESS) {
        kfree(message);
        return MACH_SEND_INVALID_DEST;
    }

    // Message successfully sent
    return MACH_MSG_SUCCESS;
}

mach_msg_return_t mach_msg_receive(mach_msg_header_t *msg, mach_msg_option_t option,
                                   mach_msg_size_t rcv_size, mach_port_name_t rcv_name,
                                   mach_msg_timeout_t timeout) {

    FIXME("does not handle option and timeout at all");

    (void)option;
    (void)timeout;

    if (msg == NULL || rcv_size < sizeof(mach_msg_header_t) || rcv_size > MACH_MSG_MAX_SIZE) {
        return MACH_RCV_INVALID_DATA;
    }

    // Look up the receive right for the specified port name
    mach_msg_return_t error;
    ipc_port_t *port = lookup_port(rcv_name, MACH_PORT_RIGHT_RECEIVE, &error);
    if (port == NULL) {
        return error;
    }

    // Dequeue a message from the port's message queue
    mach_kmsg_t *message = NULL;
    kern_return_t result = ipc_port_dequeue_kmsg(port, &message);
    if (result != KERN_SUCCESS) {
        ipc_port_release(port);
        return MACH_RCV_PORT_DIED;
    }
    if (message == NULL) {
        ipc_port_release(port);
        return MACH_RCV_TIMED_OUT;
    }

    // Validate the size of the received message
    mach_msg_size_t message_size = message->header->msgh_size;
    if (message_size < sizeof(mach_msg_header_t) || message_size > MACH_MSG_MAX_SIZE) {
        kfree(message);
        ipc_port_release(port);
        return MACH_RCV_INVALID_DATA;
    }
    if (message_size > rcv_size) {
        requeue_message_front(port, message);
        ipc_port_release(port);
        return MACH_RCV_TOO_LARGE;
    }
    if (pmap_copy_to_user((uintptr_t)msg, message->data, message_size) != 0) {
        requeue_message_front(port, message);
        ipc_port_release(port);
        return MACH_RCV_INVALID_DATA;
    }

    // Cleanup: free the message and release the port
    kfree(message);
    ipc_port_release(port);

    // Message successfully received
    return MACH_MSG_SUCCESS;
}