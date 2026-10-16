#ifndef CELESTEOS_MACH_PORT_SPACE_H
#define CELESTEOS_MACH_PORT_SPACE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <kernel/spinlock.h>
#include <mach/kern_return.h>
#include <mach/mach_types.h>

#define MACH_PORT_RIGHT_NONE      0x00
#define MACH_PORT_RIGHT_SEND      (1 << 0)
#define MACH_PORT_RIGHT_RECEIVE   (1 << 1)
#define MACH_PORT_RIGHT_SEND_ONCE (1 << 2)
#define MACH_PORT_RIGHT_PORT_SET  (1 << 3)

/* Forward declaration */
typedef struct ipc_port ipc_port_t;

/**
 * A single entry in the Mach port table.
 */
typedef struct mach_port_entry {
    mach_port_name_t name;   /* the name of the port */
    mach_port_right_t right; /* the associated port right (e.g., send, receive) */
    ipc_port_t* port;
} mach_port_entry_t;

/**
 * The Mach port table, which holds all port entries.
 */
typedef struct mach_port_table {
    mach_port_entry_t* entries;
    uint32_t count;
} mach_port_table_t;

/**
 * The Mach port space, which contains the port table and a lock.
 */
typedef struct mach_port_space {
    spinlock_t lock;
    mach_port_table_t table;
} mach_port_space_t;

/**
 * Allocate a new Mach port space and initialize its lock and port table.
 *
 * Returns a pointer to the newly created Mach port space, or NULL if allocation fails.
 */
kern_return_t mach_port_space_create(mach_port_space_t** out_space);

/**
 * Destroy the given Mach port space and release its resources.
 */
kern_return_t mach_port_space_destroy(mach_port_space_t* space);

/**
 * Insert an entry into the given Mach port space.
 *
 * Returns KERN_SUCCESS if the entry was successfully inserted, KERN_FAILURE otherwise.
 */
kern_return_t mach_port_space_insert(mach_port_space_t* space, ipc_port_t* port, mach_port_name_t name, mach_port_right_t right);

/**
 * Look up an entry in the given Mach port space by its name.
 *
 * Returns KERN_SUCCESS if the entry was found, KERN_FAILURE otherwise. 
 * If found, the associated port and right are stored in the output parameters.
 */
kern_return_t mach_port_space_lookup(mach_port_space_t* space, mach_port_name_t name, ipc_port_t** out_port, mach_port_right_t* out_right);

#endif  // CELESTEOS_MACH_PORT_SPACE_H