#include "ipc_space.h"

#include <kernel/kmalloc.h>
#include <string.h>

static kern_return_t ipc_space_insert_locked(ipc_space_t *space, ipc_object_t *object,
                                             mach_port_name_t name, mach_port_right_t right) {

    // Check if the name already exists in the IPC space.
    for (uint32_t i = 0; i < space->table.count; ++i) {
        if (space->table.entries[i].name == name) {
            return KERN_ALREADY_EXISTS;
        }
    }

    // We ran out of available names
    if (space->table.count == UINT32_MAX) {
        return KERN_NO_MEMORY;
    }

    // Insert the entry if we can
    size_t count = space->table.count + 1;
    mach_port_entry_t *entries = kmalloc(count * sizeof(*entries));
    if (entries == NULL) {
        return KERN_NO_MEMORY;
    }
    if (space->table.count != 0) {
        memcpy(entries, space->table.entries, space->table.count * sizeof(*entries));
    }

    // Try to increment the entry's reference count before inserting it into the table.
    kern_return_t result = ipc_object_try_reference(object);
    if (result != KERN_SUCCESS) {
        kfree(entries);
        return result;
    }
    entries[space->table.count] = (mach_port_entry_t){
        .name = name,
        .right = right,
        .object = object,
    };

    // Swap the old table with the new one.
    kfree(space->table.entries);
    space->table.entries = entries;
    space->table.count++;
    return KERN_SUCCESS;
}

kern_return_t ipc_space_create(ipc_space_t **out_space) {
    if (out_space == NULL) {
        return KERN_INVALID_ARGUMENT;
    }
    *out_space = NULL;

    // Allocate memory for the new IPC space
    ipc_space_t *space = kmalloc(sizeof(ipc_space_t));

    if (!space) {
        return KERN_NO_MEMORY;
    }

    // Initialize the IPC space
    spinlock_init(&space->lock);
    space->table.entries = NULL;
    space->table.count = 0;
    space->next_name = 100;

    *out_space = space;
    return KERN_SUCCESS;
}

kern_return_t ipc_space_destroy(ipc_space_t *space) {
    if (!space) {
        return KERN_INVALID_ARGUMENT;
    }

    // Lock, copy the table entries and count, then release the lock before destroying the entries.
    spinlock_acquire(&space->lock);
    mach_port_entry_t *entries = space->table.entries;
    uint32_t count = space->table.count;
    space->table.entries = NULL;
    space->table.count = 0;
    spinlock_release(&space->lock);

    // Release every IPC object referenced
    for (uint32_t i = 0; i < count; ++i) {
        ipc_object_release(entries[i].object);
    }
    kfree(entries);
    kfree(space);
    return KERN_SUCCESS;
}

kern_return_t ipc_space_insert(ipc_space_t *space, ipc_object_t *object, mach_port_name_t name,
                               mach_port_right_t right) {

    // Abort if the space is NULL, the object is invalid, the port name is either NULL or a dead
    // name, or the port right is invalid.
    if (space == NULL || !IO_VALID(object) || name == MACH_PORT_NULL || name == MACH_PORT_DEAD ||
        right > MACH_PORT_RIGHT_DEAD_NAME) {
        return KERN_INVALID_ARGUMENT;
    }

    // Acquire the lock, attempt to insert the object, and release the lock.
    spinlock_acquire(&space->lock);
    kern_return_t result = ipc_space_insert_locked(space, object, name, right);
    spinlock_release(&space->lock);
    return result;
}

kern_return_t ipc_space_insert_anon(ipc_space_t *space, ipc_object_t *object,
                                    mach_port_right_t right, mach_port_name_t *out_name) {
    if (space == NULL || !IO_VALID(object) || out_name == NULL ||
        right > MACH_PORT_RIGHT_DEAD_NAME) {
        return KERN_INVALID_ARGUMENT;
    }
    *out_name = MACH_PORT_NULL;

    spinlock_acquire(&space->lock);
    mach_port_name_t candidate = space->next_name;
    if (candidate == MACH_PORT_NULL || candidate == MACH_PORT_DEAD) {
        candidate = 100;
    }
    kern_return_t result = KERN_NO_MEMORY;
    uint32_t attempts = space->table.count + 1;
    for (uint32_t i = 0; i < attempts; ++i) {
        if (candidate == MACH_PORT_NULL || candidate == MACH_PORT_DEAD) {
            candidate = 100;
        }
        result = ipc_space_insert_locked(space, object, candidate, right);
        if (result == KERN_SUCCESS) {
            *out_name = candidate;
            space->next_name = candidate + 1;
            break;
        }
        if (result != KERN_ALREADY_EXISTS) {
            break;
        }
        candidate++;
    }
    spinlock_release(&space->lock);
    return result;
}

kern_return_t ipc_space_lookup(ipc_space_t *space, mach_port_name_t name, ipc_object_t **out_object,
                               mach_port_right_t *out_right) {

    // Abort if the space is NULL, the name is invalid, or the output parameters are NULL.
    if (space == NULL || name == MACH_PORT_NULL || out_object == NULL || out_right == NULL) {
        return KERN_INVALID_ARGUMENT;
    }

    // Initialize the output parameters to indicate no object and a dead name.
    *out_object = NULL;
    *out_right = MACH_PORT_RIGHT_DEAD_NAME;

    // Acquire the lock and search for the entry with the given name.
    spinlock_acquire(&space->lock);
    for (uint32_t i = 0; i < space->table.count; ++i) {
        // Grab the entry at the current index.
        mach_port_entry_t *entry = &space->table.entries[i];

        // Compare the entry against the port name we are looking up
        if (entry->name == name) {
            // Try to reference the object, to ensure it is valid
            kern_return_t result = ipc_object_try_reference(entry->object);
            if (result != KERN_SUCCESS) {
                spinlock_release(&space->lock);
                return result;
            }

            // Found the object, so we have to set the output parameters accordingly.
            *out_object = entry->object;
            *out_right = entry->right;
            spinlock_release(&space->lock);
            return KERN_SUCCESS;
        }
    }
    spinlock_release(&space->lock);
    return KERN_NOT_FOUND;
}

kern_return_t ipc_space_remove(ipc_space_t *space, mach_port_name_t name) {
    if (space == NULL || name == MACH_PORT_NULL) {
        return KERN_INVALID_ARGUMENT;
    }

    // Acquire the lock and search for the entry with the given name.
    ipc_object_t *object = NULL;
    spinlock_acquire(&space->lock);
    for (uint32_t i = 0; i < space->table.count; ++i) {
        // Skip entries that do not match the name we are looking for.
        if (space->table.entries[i].name != name) {
            continue;
        }

        //
        object = space->table.entries[i].object;
        space->table.count--;
        if (i != space->table.count) {
            space->table.entries[i] = space->table.entries[space->table.count];
        }
        spinlock_release(&space->lock);
        ipc_object_release(object);
        return KERN_SUCCESS;
    }
    spinlock_release(&space->lock);
    return KERN_NOT_FOUND;
}