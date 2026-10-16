#ifndef CELESTEOS_MACH_PORT_H
#define CELESTEOS_MACH_PORT_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include <kernel/spinlock.h>

typedef struct thread thread_t;
typedef uint32_t mach_port_name_t;
typedef uint32_t mach_port_right_t;

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
ipc_port_t* ipc_port_alloc(void);

/**
 * Increment the reference count of the given Mach IPC port.
 */
void ipc_port_reference(ipc_port_t* port);

/**
 * Decrement the reference count of the given Mach IPC port.
 * If the reference count reaches zero, the port is deallocated.
 */
void ipc_port_release(ipc_port_t* port);

/**
 * Destroy the given Mach IPC port.
 * NOTE: This function should only be called when the port is no longer referenced.
 */
void ipc_port_destroy(ipc_port_t* port);

/**
 * Enqueue a Mach kernel message to the given Mach IPC port's message queue.
 * 
 * Returns true if the message was successfully enqueued, false otherwise.
 */
bool ipc_port_enqueue_kmsg(ipc_port_t* port, mach_kmsg_t* kmsg);

/**
 * Dequeue a Mach kernel message from the given Mach IPC port's message queue.
 * 
 * Returns the dequeued message, or NULL if the queue is empty.
 */
mach_kmsg_t* ipc_port_dequeue_kmsg(ipc_port_t* port);

#endif  // CELESTEOS_MACH_PORT_H