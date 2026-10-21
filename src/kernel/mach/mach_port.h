#ifndef CELESTEOS_MACH_PORT_H
#define CELESTEOS_MACH_PORT_H

#include <ipc/ipc_port.h>
#include <ipc/ipc_space.h>
#include <mach/kern_return.h>
#include <mach/mach_types.h>

typedef ipc_space_t mach_port_space_t;
typedef ipc_port_t mach_port_t;

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
 * Returns KERN_SUCCESS on enqueue or a kernel error code.
 */
kern_return_t mach_port_enqueue_kmsg(mach_port_t *port, mach_kmsg_t *kmsg);

/**
 * Dequeue a Mach kernel message from the given Mach IPC port's message queue.
 *
 * On KERN_SUCCESS, stores the dequeued message in `out_kmsg`, or NULL if the queue is empty.
 */
kern_return_t mach_port_dequeue_kmsg(mach_port_t *port, mach_kmsg_t **out_kmsg);

#endif // CELESTEOS_MACH_PORT_H