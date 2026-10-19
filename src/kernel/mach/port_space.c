#include "port_space.h"

#include <kernel/kmalloc.h>

kern_return_t port_space_create(mach_port_space_t **out_space) {
    mach_port_space_t *space = kmalloc(sizeof(mach_port_space_t));

    // Failed to allocate memory for the port space
    if (!space) {
        return KERN_FAILURE;
    }

    // Initialize the port space structure
    spinlock_init(&space->lock);
    space->table.entries = NULL;
    space->table.count = 0;

    *out_space = space;
    return KERN_SUCCESS;
}

kern_return_t port_space_destroy(mach_port_space_t *space) {
    // Cannot free a NULL mach_port_space
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

    // Free the mach_port_space structure itself
    kfree(space);
    return KERN_SUCCESS;
}

kern_return_t port_space_insert(mach_port_space_t *space, mach_port_t *port, mach_port_name_t name,
                                mach_port_right_t right) {
    // STUB: Implement Mach port space insertion logic here.
    STUB("port_space_insert");
}

kern_return_t port_space_lookup(mach_port_space_t *space, mach_port_name_t name,
                                mach_port_t **out_port, mach_port_right_t *out_right) {
    // STUB: Implement Mach port space lookup logic here.
    STUB("port_space_lookup");
}