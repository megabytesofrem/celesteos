#include "port.h"

#include <kernel/memory/kmalloc.h>

kern_return_t ipc_port_allocate(mach_port_space_t *space, uint32_t flags,
                                mach_port_name_t *out_name, ipc_port_t **port) {
    // STUB: Implement IPC port allocation logic here.
    STUB("ipc_port_allocate");
}

kern_return_t ipc_port_reference(ipc_port_t *port) {
    // STUB: Implement IPC port reference logic here.
    port->ref_count++;

    return KERN_SUCCESS;
}

kern_return_t ipc_port_release(ipc_port_t *port) {
    if (!port) {
        return KERN_INVALID_ARGUMENT;
    }

    spinlock_acquire(&port->lock);

    if (port->ref_count > 0) {
        port->ref_count--;
    }

    spinlock_release(&port->lock);

    // If the reference count reaches zero, destroy the port.
    if (port->ref_count == 0) {
        ipc_port_destroy(port);
    }

    return KERN_SUCCESS;
}

kern_return_t ipc_port_destroy(ipc_port_t *port) {
    if (!port) {
        return KERN_INVALID_ARGUMENT;
    }

    // Acquire lock to prevent concurrent enqueue/dequeue operations while destroying
    spinlock_acquire(&port->lock);

    // STUB: Flush remaining mach_kmsg_t messages from port's queue here!
    STUB("ipc_port_destroy: flush remaining messages");

    spinlock_release(&port->lock);

    // Free kernel memory allocation
    kfree(port); // Using kernel allocator (or free)
    return KERN_SUCCESS;
}

kern_return_t ipc_port_enqueue_kmsg(ipc_port_t *port, mach_kmsg_t *kmsg) {
    if (!port || !kmsg) {
        return KERN_INVALID_ARGUMENT;
    }

    spinlock_acquire(&port->lock);
    port->is_locked = true;

    // Enqueue the message to the port's message queue.
    kmsg->next = NULL;

    // Set kmsg->prev to point to the current tail of the queue.
    kmsg->prev = port->msg_queue_tail;

    if (port->msg_queue_tail != NULL) {
        // Link the new message to the end of the queue.
        port->msg_queue_tail->next = kmsg;
    } else {
        // The queue was empty, so the new message becomes the head of the queue.
        port->msg_queue_head = kmsg;
    }

    // Update the tail of the queue to the new message.
    port->msg_queue_tail = kmsg;

    // Increment the message count for the port.
    port->msg_count++;

    port->is_locked = false;
    spinlock_release(&port->lock);

    return KERN_SUCCESS;
}

kern_return_t ipc_port_dequeue_kmsg(ipc_port_t *port, mach_kmsg_t **kmsg) {
    // STUB: Implement IPC port dequeue logic here.

    if (!port || !kmsg) {
        return KERN_INVALID_ARGUMENT;
    }

    spinlock_acquire(&port->lock);
    port->is_locked = true;

    STUB("ipc_port_dequeue_kmsg");

    port->is_locked = false;
    spinlock_release(&port->lock);

    return KERN_SUCCESS;
}