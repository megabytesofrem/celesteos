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
    io_lock_init(&port->io);
    port->io.ref_count = 1;
    port->io.bits = io_makebits(true, IO_TYPE_PORT, 0);
    port->msg_queue_head = NULL;
    port->msg_queue_tail = NULL;
    port->msg_count = 0;
    port->blocked_thread = NULL;

    /* Name allocation is not implemented; do not publish an unnamed port. */
    (void)flags;
    *namep = MACH_PORT_NULL;
    *portp = NULL;
    ipc_port_release(port);
    klog(KLOG_ERROR, "ipc_port_alloc: IPC name allocation is not implemented\n");
    return KERN_FAILURE;
}

void ipc_port_dealloc(ipc_port_t *port) {
    if (port == NULL) {
        return;
    }

    // Free the port, releasing its memory
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

kern_return_t ipc_port_enqueue_kmsg(ipc_port_t *port, mach_kmsg_t *kmsg) {
    if (!port || !kmsg) {
        return KERN_INVALID_ARGUMENT;
    }

    /**
     * FIXME:
     *
     * This code is horribly, horribly unsafe, and violates all manner of thread safety.
     * It is the definition of "we just want this shit working, YOLO it"
     *
     * It has the following *amazing* properties:
     *
     * Returning `KERN_SUCCESS` when there is no guarantee the message was enqueued/dequeued
     * properly.
     *
     * Using a spin lock does not account for the case where there is no data in the queue,
     * potentially leading to a NULL pointer (thanks Gemini)
     *
     * The manual doubly linked list management is an absolute coding warcrime, and is very
     * _very_ easy to fuck up (ask me how I know...)
     *
     * The solution is using a wait queue so threads can safely wait for messages to arrive.
     */

    ip_lock(port);

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

    ip_unlock(port);

    return KERN_SUCCESS;
}

kern_return_t ipc_port_dequeue_kmsg(ipc_port_t *port, mach_kmsg_t **out_kmsg) {
    if (!port || !out_kmsg) {
        return KERN_INVALID_ARGUMENT;
    }

    /**
     * FIXME:
     *
     * This code is horribly, horribly unsafe, and violates all manner of thread safety.
     * It is the definition of "we just want this shit working, YOLO it"
     *
     * It has the following *amazing* properties:
     *
     * Returning `KERN_SUCCESS` when there is no guarantee the message was enqueued/dequeued
     * properly.
     *
     * Using a spin lock does not account for the case where there is no data in the queue,
     * potentially leading to a NULL pointer (thanks Gemini)
     *
     * The manual doubly linked list management is an absolute coding warcrime, and is very
     * _very_ easy to fuck up (ask me how I know...)
     *
     * The solution is using a wait queue so threads can safely wait for messages to arrive.
     */

    ip_lock(port);

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

    ip_unlock(port);

    return KERN_SUCCESS;
}