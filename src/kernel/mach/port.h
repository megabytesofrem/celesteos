#ifndef CELESTEOS_MACH_PORT_H
#define CELESTEOS_MACH_PORT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <kernel/spinlock.h>
#include <mach/kern_return.h>
#include <mach/mach_types.h>

typedef struct thread thread_t;
typedef struct mach_port_space mach_port_space_t;

typedef struct mach_kmsg {
    struct mach_kmsg* next;
    struct mach_kmsg* prev;
    uint32_t size;  /* size of the message */
    uint8_t data[]; /* the message data */
} mach_kmsg_t;

/**
 * A Mach IPC port.
 */
typedef struct ipc_port {
    spinlock_t lock;
    uint32_t ref_count; /* reference count for the port */
    bool is_alive;      /* indicates if the port is currently alive */

    /* Message queue */
    mach_kmsg_t* msg_head; /* head of the queue */
    mach_kmsg_t* msg_tail; /* tail of the queue */
    uint32_t msg_count;    /* number of messages in the queue */

    thread_t* blocked_thread; /* thread blocking on the port */
} ipc_port_t;

/**
 * Allocate a new Mach IPC port.
 */
kern_return_t ipc_port_alloc(mach_port_space_t* space,
                             uint32_t flags,
                             mach_port_name_t* out_name,
                             ipc_port_t** out_port);

/**
 * Increment the reference count of the given Mach IPC port.
 */
kern_return_t ipc_port_reference(ipc_port_t* port);

/**
 * Decrement the reference count of the given Mach IPC port.
 * If the reference count reaches zero, the port is deallocated.
 */
kern_return_t ipc_port_release(ipc_port_t* port);

/**
 * Destroy the given Mach IPC port.
 * NOTE: This function should only be called when the port is no longer referenced.
 */
kern_return_t ipc_port_destroy(ipc_port_t* port);

/**
 * Enqueue a Mach kernel message to the given Mach IPC port's message queue.
 * 
 * Returns true if the message was successfully enqueued, false otherwise.
 */
kern_return_t ipc_port_enqueue_kmsg(ipc_port_t* port, mach_kmsg_t* kmsg);

/**
 * Dequeue a Mach kernel message from the given Mach IPC port's message queue.
 * 
 * Returns the dequeued message, or NULL if the queue is empty.
 */
kern_return_t ipc_port_dequeue_kmsg(ipc_port_t* port, mach_kmsg_t** out_kmsg);

#endif  // CELESTEOS_MACH_PORT_H