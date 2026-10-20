#ifndef CELESTEOS_MACH_PORT_H
#define CELESTEOS_MACH_PORT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <ipc/ipc_object.h>
#include <mach/kern_return.h>
#include <mach/mach_types.h>

typedef struct thread thread_t;
typedef struct ipc_port_space mach_port_space_t;

typedef struct mach_kmsg {
    struct mach_kmsg *next;
    struct mach_kmsg *prev;
    struct mach_msg_header *header; /* Mach message header */
    uint8_t data[];                 /* the message data */
} mach_kmsg_t;

/**
 * A Mach IPC port.
 */
typedef struct mach_port {
    ipc_object_t io; /* common object lock, references, and type/state bits */

    bool is_locked; /* indicates if the port is currently locked */
    bool is_alive;  /* indicates if the port is currently alive */

    /* Message queue */
    mach_kmsg_t *msg_queue_head; /* head of the queue */
    mach_kmsg_t *msg_queue_tail; /* tail of the queue */
    uint32_t msg_count;          /* number of messages in the queue */

    thread_t *blocked_thread; /* thread blocking on the port */
} mach_port_t;

_Static_assert(offsetof(mach_port_t, io) == 0, "IPC object must be the port's first member");

/**
 * Allocate a new Mach IPC port by creating a new port right in the specified port space
 */
kern_return_t mach_port_allocate(mach_port_space_t *space, mach_port_right_t right,
                                 mach_port_name_t *out_name);

/**
 * Increment the reference count of the given Mach IPC port.
 */
kern_return_t mach_port_reference(mach_port_t *port);

/**
 * Decrement the reference count of the given Mach IPC port.
 * If the reference count reaches zero, the port is deallocated.
 */
kern_return_t mach_port_release(mach_port_t *port);

/**
 * Destroy the given Mach IPC port.
 * NOTE: This function should only be called when the port is no longer referenced.
 */
kern_return_t mach_port_destroy(mach_port_t *port);

/**
 * Enqueue a Mach kernel message to the given Mach IPC port's message queue.
 *
 * Returns true if the message was successfully enqueued, false otherwise.
 */
kern_return_t mach_port_enqueue_kmsg(mach_port_t *port, mach_kmsg_t *kmsg);

/**
 * Dequeue a Mach kernel message from the given Mach IPC port's message queue.
 *
 * Returns the dequeued message, or NULL if the queue is empty.
 */
kern_return_t mach_port_dequeue_kmsg(mach_port_t *port, mach_kmsg_t **out_kmsg);

#endif // CELESTEOS_MACH_PORT_H