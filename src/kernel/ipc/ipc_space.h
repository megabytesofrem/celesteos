/**
 * File: ipc/ipc_space.h
 *
 * Internal IPC spaces.
 */

#ifndef CELESTEOS_IPC_SPACE_H
#define CELESTEOS_IPC_SPACE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <ipc/ipc_object.h>
#include <kernel/mach/kern_return.h>
#include <kernel/mach/mach_types.h>
#include <kernel/spinlock.h>

/**
 * A single entry in the Mach port table.
 */
typedef struct mach_port_entry {
    mach_port_name_t name;   /* the name of the port */
    mach_port_right_t right; /* the associated port right (e.g., send, receive) */
    ipc_object_t *object; /* port or port-set object; not an embedded object */
} mach_port_entry_t;

/**
 * The Mach port table, which holds all port entries.
 */
typedef struct mach_port_table {
    mach_port_entry_t *entries;
    uint32_t count;
} mach_port_table_t;

/**
 * The IPC space which contains a Mach port table and a lock.
 */
typedef struct ipc_space {
    spinlock_t lock;
    mach_port_table_t table;
} ipc_space_t;

/**
 * Allocate a new IPC space and initialize its lock and port table.
 *
 * Returns a pointer to the newly created IPC space, or NULL if allocation fails.
 */
kern_return_t ipc_space_create(ipc_space_t **out_space);

/**
 * Destroy the given IPC space and release its resources.
 */
kern_return_t ipc_space_destroy(ipc_space_t *space);

/**
 * Insert an entry into the given IPC space.
 *
 * Returns KERN_SUCCESS if the entry was successfully inserted, KERN_FAILURE otherwise.
 */
kern_return_t ipc_space_insert(ipc_space_t *space, ipc_object_t *object, mach_port_name_t name,
                               mach_port_right_t right);

/**
 * Look up an entry in the given IPC space by its name.
 *
 * Returns KERN_SUCCESS if the entry was found, KERN_FAILURE otherwise.
 * If found, the associated object and right are stored in the output parameters.
 */
kern_return_t ipc_space_lookup(ipc_space_t *space, mach_port_name_t name, ipc_object_t **out_object,
                               mach_port_right_t *out_right);

#endif // CELESTEOS_IPC_SPACE_H