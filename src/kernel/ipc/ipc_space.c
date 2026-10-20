#include "ipc_space.h"

#include <kernel/kmalloc.h>

kern_return_t ipc_space_create(ipc_space_t **out_space) {
    ipc_space_t *space = kmalloc(sizeof(ipc_space_t));

    // Failed to allocate memory for the ipc_space_t structure
    if (!space) {
        return KERN_FAILURE;
    }

    // Initialize the ipc_space_t structure
    spinlock_init(&space->lock);
    space->table.entries = NULL;
    space->table.count = 0;

    *out_space = space;
    return KERN_SUCCESS;
}

kern_return_t ipc_space_destroy(ipc_space_t *space) {
    // Cannot free a NULL ipc_space_t
    if (!space) {
        return KERN_FAILURE;
    }

    spinlock_acquire(&space->lock);
    // Free port table entries
    if (space->table.entries) {
        kfree(space->table.entries);
        space->table.entries = NULL;
        space->table.count = 0;
    }
    spinlock_release(&space->lock);

    // Free the ipc_space_t structure itself
    kfree(space);
    return KERN_SUCCESS;
}

kern_return_t ipc_space_insert(ipc_space_t *space, ipc_object_t *object, mach_port_name_t name,
                               mach_port_right_t right) {
    // STUB: Implement IPC space insertion logic here.
    STUB("ipc_space_insert");
}

kern_return_t ipc_space_lookup(ipc_space_t *space, mach_port_name_t name, ipc_object_t **out_object,
                               mach_port_right_t *out_right) {
    // STUB: Implement IPC space lookup logic here.
    STUB("ipc_space_lookup");
}