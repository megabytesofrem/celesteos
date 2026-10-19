/**
 * ipc/ipc_space.h
 *
 * Internal IPC spaces.
 */

#ifndef CELESTEOS_IPC_SPACE_H
#define CELESTEOS_IPC_SPACE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <kernel/mach/kern_return.h>
#include <kernel/mach/mach_types.h>
#include <kernel/spinlock.h>

#define MACH_PORT_RIGHT_NONE 0x00          /* Port has no rights */
#define MACH_PORT_RIGHT_SEND (1 << 0)      /* Port can send messages */
#define MACH_PORT_RIGHT_RECEIVE (1 << 1)   /* Port can receive messages */
#define MACH_PORT_RIGHT_SEND_ONCE (1 << 2) /* Port can send a message only once */
#define MACH_PORT_RIGHT_PORT_SET (1 << 3)  /* Port set rights */

/* Forward declaration */
typedef struct mach_port mach_port_t;

/**
 * A single entry in the Mach port table.
 */
typedef struct mach_port_entry {
    mach_port_name_t name;   /* the name of the port */
    mach_port_right_t right; /* the associated port right (e.g., send, receive) */
    mach_port_t *port;
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
kern_return_t ipc_space_insert(ipc_space_t *space, mach_port_t *port, mach_port_name_t name,
                               mach_port_right_t right);

/**
 * Look up an entry in the given IPC space by its name.
 *
 * Returns KERN_SUCCESS if the entry was found, KERN_FAILURE otherwise.
 * If found, the associated port and right are stored in the output parameters.
 */
kern_return_t ipc_space_lookup(ipc_space_t *space, mach_port_name_t name, mach_port_t **out_port,
                               mach_port_right_t *out_right);

#endif // CELESTEOS_IPC_SPACE_H