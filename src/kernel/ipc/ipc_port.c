#include "ipc_port.h"

#include <kernel/kmalloc.h>

kern_return_t ipc_port_alloc(ipc_space_t *space, ipc_port_init_flags_t flags,
                             mach_port_name_t *namep, ipc_port_t **portp) {
    if (space == NULL || namep == NULL || portp == NULL) {
        return KERN_INVALID_ARGUMENT;
    }

    // Allocate raw kernel port object from heap
    struct ipc_port *port = (struct ipc_port *)kmalloc(sizeof(struct ipc_port));
    if (port == NULL) {
        return KERN_NO_MEMORY;
    }

    // Initialize port fields and locks
    spinlock_init(&port->lock);
    port->ref_count = 1;
    port->is_locked = false;
    port->is_alive = true;
    port->msg_queue_head = NULL;
    port->msg_queue_tail = NULL;
    port->msg_count = 0;
    port->blocked_thread = NULL;

    // Allocate a port name entry inside the target task's IPC space
    mach_port_name_t name;

    STUB("ipc_port_alloc: ipc_entry_alloc inner");

    // kern_return_t kr = ipc_space_entry_alloc(space, port, &name);
    // if (kr != KERN_SUCCESS) {
    //     kfree(port);
    //     return kr;
    // }

    // Return both the handle name and the kernel object pointer
    *namep = name;
    *portp = port;

    return KERN_SUCCESS;
}

void ipc_port_dealloc(ipc_port_t *port) {
    if (port == NULL) {
        return;
    }

    // Release the lock if it is still held
    if (port->is_locked) {
        spinlock_release(&port->lock);
    }

    // Free the port, releasing its memory
    kfree(port);
}

void ipc_port_reference(ipc_port_t *port) {
    if (port == NULL) {
        return;
    }

    spinlock_acquire(&port->lock);

    // Increment the reference count to retain
    port->ref_count++;
    spinlock_release(&port->lock);
}

void ipc_port_release(ipc_port_t *port) {
    if (port == NULL) {
        return;
    }

    spinlock_acquire(&port->lock);

    // Decrement the reference count to release
    port->ref_count--;

    if (port->ref_count == 0) {
        spinlock_release(&port->lock);
        ipc_port_dealloc(port);
    } else {
        spinlock_release(&port->lock);
    }
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

kern_return_t ipc_port_dequeue_kmsg(ipc_port_t *port, mach_kmsg_t **out_kmsg) {
    if (!port || !out_kmsg) {
        return KERN_INVALID_ARGUMENT;
    }

    spinlock_acquire(&port->lock);
    port->is_locked = true;

    // Grab the head of the message queue.
    *out_kmsg = port->msg_queue_head;

    if (*out_kmsg) {
        // Unlink the message from the head of the queue.
        port->msg_queue_head = (*out_kmsg)->next;

        // If the queue is not empty after removing the head, update it's
        // prev pointer to NULL, since it is now the head of the queue.
        if (port->msg_queue_head) {
            // The new head of the queue
            port->msg_queue_head->prev = NULL;
        } else {
            // The queue is now empty, so it has no head or tail.
            port->msg_queue_tail = NULL;
        }

        // Decrement the message count for the port, as we dequeue
        port->msg_count--;
    }

    port->is_locked = false;
    spinlock_release(&port->lock);

    return KERN_SUCCESS;
}