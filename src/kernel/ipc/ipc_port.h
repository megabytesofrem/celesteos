/**
 * File: ipc/ipc_port.h
 *
 * Internal kernel IPC ports.
 */

#ifndef CELESTEOS_IPC_PORT_H
#define CELESTEOS_IPC_PORT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <ipc/ipc_object.h>
#include <kernel/spinlock.h>
#include <mach/kern_return.h>
#include <mach/mach_port.h>
#include <mach/mach_types.h>

typedef struct thread thread_t;
typedef struct ipc_space ipc_space_t;

typedef int ipc_port_init_flags_t;

/**
 * Internal IPC port structure.
 */
typedef struct ipc_port {
    ipc_object_t io;

    /* Message queue */
    mach_kmsg_t *msg_queue_head;
    mach_kmsg_t *msg_queue_tail;
    uint32_t msg_count;

    thread_t *blocked_thread;
} ipc_port_t;

#define ip_active(port) io_active(&(port)->io)
#define ip_lock(port) io_lock(&(port)->io)
#define ip_unlock(port) io_unlock(&(port)->io)

/* Internal Kernel Port Primitives */

/**
 * Allocate a kernel port and a name in the target IPC space.
 */
kern_return_t ipc_port_alloc(ipc_space_t *space, ipc_port_init_flags_t flags,
                             mach_port_name_t *namep, ipc_port_t **portp);

/**
 * Free a raw kernel port object back to the heap.
 */
void ipc_port_dealloc(ipc_port_t *port);

/**
 * Retain an internal pointer reference to the kernel port.
 */
void ipc_port_reference(ipc_port_t *port);

/**
 * Release an internal pointer reference to the kernel port.
 * The message queue must be drained before releasing the last reference.
 */
void ipc_port_release(ipc_port_t *port);

/**
 * Internal message queue routines
 */
kern_return_t ipc_port_enqueue_kmsg(ipc_port_t *port, mach_kmsg_t *kmsg);
kern_return_t ipc_port_dequeue_kmsg(ipc_port_t *port, mach_kmsg_t **out_kmsg);

#endif // CELESTEOS_IPC_PORT_H