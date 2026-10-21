#include "ipc_port.h"

#include <kernel/kmalloc.h>
#include <mach/mach_port.h>

#define IPC_PORT_MAX_QUEUED_MESSAGES 256

kern_return_t ipc_port_alloc(ipc_space_t *space, ipc_port_init_flags_t flags,
                             mach_port_name_t *namep, ipc_port_t **portp) {
    if (space == NULL || namep == NULL || portp == NULL) {
        return KERN_INVALID_ARGUMENT;
    }
    *namep = MACH_PORT_NULL;
    *portp = NULL;
    if (flags != 0) {
        return KERN_INVALID_ARGUMENT;
    }

    ipc_object_t *object;
    kern_return_t result =
        ipc_object_alloc(space, IO_TYPE_PORT, MACH_PORT_TYPE_RECEIVE, 1, namep, &object);
    if (result != KERN_SUCCESS) {
        return result;
    }
    *portp = (ipc_port_t *)object;
    return KERN_SUCCESS;
}

void ipc_port_dealloc(ipc_port_t *port) {
    if (port == NULL) {
        return;
    }

    ip_lock(port);
    mach_kmsg_t *message = port->msg_queue_head;
    port->msg_queue_head = NULL;
    port->msg_queue_tail = NULL;
    port->msg_count = 0;
    ip_unlock(port);

    while (message != NULL) {
        mach_kmsg_t *next = message->next;
        kfree(message);
        message = next;
    }
    kfree(port);
}

void ipc_port_reference(ipc_port_t *port) {
    if (port == NULL) {
        return;
    }

    ipc_object_reference(&port->io);
}

void ipc_port_release(ipc_port_t *port) {
    if (port == NULL) {
        return;
    }

    ipc_object_release(&port->io);
}

kern_return_t mach_port_allocate(mach_port_space_t *space, mach_port_right_t right,
                                 mach_port_name_t *out_name) {
    if (space == NULL || out_name == NULL || right != MACH_PORT_RIGHT_RECEIVE) {
        return KERN_INVALID_ARGUMENT;
    }
    ipc_port_t *port;
    kern_return_t result = ipc_port_alloc(space, 0, out_name, &port);
    if (result == KERN_SUCCESS) {
        ipc_port_release(port);
    }
    return result;
}

kern_return_t mach_port_reference(mach_port_t *port) {
    if (port == NULL) {
        return KERN_INVALID_ARGUMENT;
    }
    return ipc_object_try_reference(&port->io);
}

kern_return_t mach_port_release(mach_port_t *port) {
    if (port == NULL) {
        return KERN_INVALID_ARGUMENT;
    }
    ipc_port_release(port);
    return KERN_SUCCESS;
}

kern_return_t mach_port_destroy(mach_port_t *port) {
    return mach_port_release(port);
}

kern_return_t ipc_port_enqueue_kmsg(ipc_port_t *port, mach_kmsg_t *kmsg) {
    if (!port || !kmsg) {
        return KERN_INVALID_ARGUMENT;
    }

    FIXME("double-linked list breaks thread safety");
    FIXME("replace with wait_queue");

    ip_lock(port);
    if (!io_active(&port->io) || port->msg_count >= IPC_PORT_MAX_QUEUED_MESSAGES) {
        ip_unlock(port);
        return KERN_NO_MEMORY;
    }
    kmsg->next = NULL;
    kmsg->prev = port->msg_queue_tail;

    if (port->msg_queue_tail != NULL) {
        port->msg_queue_tail->next = kmsg;
    } else {
        port->msg_queue_head = kmsg;
    }
    port->msg_queue_tail = kmsg;
    port->msg_count++;
    ip_unlock(port);

    return KERN_SUCCESS;
}

kern_return_t ipc_port_dequeue_kmsg(ipc_port_t *port, mach_kmsg_t **out_kmsg) {
    if (!port || !out_kmsg) {
        return KERN_INVALID_ARGUMENT;
    }

    FIXME("double-linked list breaks thread safety");
    FIXME("replace with wait_queue");

    ip_lock(port);
    *out_kmsg = port->msg_queue_head;

    if (*out_kmsg) {
        port->msg_queue_head = (*out_kmsg)->next;
        if (port->msg_queue_head) {
            port->msg_queue_head->prev = NULL;
        } else {
            port->msg_queue_tail = NULL;
        }
        port->msg_count--;
        (*out_kmsg)->next = NULL;
        (*out_kmsg)->prev = NULL;
    }
    ip_unlock(port);

    return KERN_SUCCESS;
}

kern_return_t mach_port_enqueue_kmsg(mach_port_t *port, mach_kmsg_t *kmsg) {
    return ipc_port_enqueue_kmsg(port, kmsg);
}

kern_return_t mach_port_dequeue_kmsg(mach_port_t *port, mach_kmsg_t **out_kmsg) {
    return ipc_port_dequeue_kmsg(port, out_kmsg);
}