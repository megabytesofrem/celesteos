#include "ipc_object.h"

#include <ipc/ipc_port.h>
#include <ipc/ipc_space.h>
#include <kernel/kmalloc.h>
#include <string.h>

#include <kernel/stub.h>

static bool ipc_right_from_type(mach_port_type_t type, mach_port_right_t *right) {
    switch (type) {
    case MACH_PORT_TYPE_SEND:
        *right = MACH_PORT_RIGHT_SEND;
        return true;
    case MACH_PORT_TYPE_RECEIVE:
        *right = MACH_PORT_RIGHT_RECEIVE;
        return true;
    case MACH_PORT_TYPE_SEND_ONCE:
        *right = MACH_PORT_RIGHT_SEND_ONCE;
        return true;
    default:
        return false;
    }
}

static kern_return_t ipc_object_allocate_name(ipc_space_t *space, ipc_object_type_t type,
                                              mach_port_type_t ptype, mach_port_urefs_t refs,
                                              mach_port_name_t requested_name,
                                              mach_port_name_t *name, ipc_object_t **object) {
    if (object == NULL || name == NULL) {
        return KERN_INVALID_ARGUMENT;
    }
    *object = NULL;
    *name = MACH_PORT_NULL;
    if (space == NULL || type != IO_TYPE_PORT || refs != 1) {
        return KERN_INVALID_ARGUMENT;
    }

    mach_port_right_t right;
    if (!ipc_right_from_type(ptype, &right)) {
        return KERN_INVALID_ARGUMENT;
    }

    ipc_port_t *port = kmalloc(sizeof(*port));
    if (port == NULL) {
        return KERN_NO_MEMORY;
    }
    memset(port, 0, sizeof(*port));
    io_lock_init(&port->io);
    port->io.ref_count = 1;
    port->io.bits = io_makebits(true, IO_TYPE_PORT, 0);

    kern_return_t result = requested_name == MACH_PORT_NULL
                               ? ipc_space_insert_anon(space, &port->io, right, name)
                               : ipc_space_insert(space, &port->io, requested_name, right);
    if (result != KERN_SUCCESS) {
        kfree(port);
        return result;
    }
    if (requested_name != MACH_PORT_NULL) {
        *name = requested_name;
    }
    *object = &port->io;
    return KERN_SUCCESS;
}

kern_return_t ipc_object_alloc(ipc_space_t *space, ipc_object_type_t type, mach_port_type_t ptype,
                               mach_port_urefs_t refs, mach_port_name_t *name,
                               ipc_object_t **object) {

    /**
     *
     * Implementation note:
     *
     * ************************************************************************************
     *
     * This function creates a new IPC object and sets up its initial capability in a given IPC
     * space.
     *
     * This function can either allocate a kernel object, or user-space IPC entry.
     * - Kernel object: shared lock, lifetime reference count, active bit set state, port state
     * - User-space entry: task local name, capability rights, reference count and a pointer to the
     * actual object
     *
     * `type` determines what is to be allocated (e.g a port)
     * `ptype` specifies the type of port capability to create.
     * `refs` are references beloing to the capability entry, not directly object->ref_count
     *
     * Before allocating:
     * - Validate the IPC space (pointers must be valid)
     * - Ensure the requested type and port type are supported.
     * - Check for available resources.
     *
     * Ownership:
     * - The caller takes ownership of the newly allocated object and is responsible for releasing
     * it.
     * - Locks are managed internally and should not be held by the caller when invoking this
     * function.
     * - The function may fail, in which case no ownership is transferred and the caller must handle
     * the error.
     *
     */

    if (name == NULL) {
        return KERN_INVALID_ARGUMENT;
    }
    *name = MACH_PORT_NULL;
    return ipc_object_allocate_name(space, type, ptype, refs, MACH_PORT_NULL, name, object);
}

kern_return_t ipc_object_alloc_name(ipc_space_t *space, ipc_object_type_t type,
                                    mach_port_type_t ptype, mach_port_urefs_t refs,
                                    mach_port_name_t name, ipc_object_t **object) {

    /**
     * Implementation note:
     *
     * ************************************************************
     * This function creates a new IPC object and associates it with a specified name within the
     * given IPC space.
     *
     * The behaviour should follow the general IPC object allocation detailed in `ipc_object_alloc`,
     * with the additional step of associating the object with the specified name in the IPC space.
     *
     * Ownership:
     * - The caller takes ownership of the newly allocated object and is responsible for releasing
     * it.
     * - Locks are managed internally and should not be held by the caller when invoking this
     * function.
     * - The function may fail, in which case no ownership is transferred and the caller must handle
     * the error.
     */

    if (name == MACH_PORT_NULL || object == NULL) {
        return KERN_INVALID_ARGUMENT;
    }
    *object = NULL;
    mach_port_name_t installed_name;
    return ipc_object_allocate_name(space, type, ptype, refs, name, &installed_name, object);
}

void ipc_object_destroy(ipc_object_t *object, mach_msg_type_name_t msg_type) {
    /**
     * Implementation note:
     *
     * ****************************************************************************
     *
     * This function is responsible for safely destroying an IPC object, ensuring that all resources
     * associated with the object are properly released and that the object is no longer accessible.
     *
     * Ownership:
     * - The function ensures that all internal locks are properly released before the object is
     * destroyed.
     * - Any failure in the destruction process should be logged, but this function will attempt to
     * clean up as much as possible.
     *
     * `msg_type` is specified alongside the object since different message types potentially
     * require different cleanup procedures.
     *
     * MACH_MSG_TYPE_MOVE_SEND requires transferring its send rights to another recipient or
     * releasing them appropriately. MACH_MSG_TYPE_MOVE_RECEIVE requires releasing the receive right
     * appropriately.
     *
     */

    (void)msg_type;
    FIXME("ipc_object_destroy: not handle msg_type");

    if (!IO_VALID(object)) {
        klog(KLOG_ERROR, "ipc: ipc_object_destroy: invalid object\n");
        return;
    }
    ipc_object_release(object);
}

void ipc_object_reference(ipc_object_t *object) {
    kern_return_t result = ipc_object_try_reference(object);
    if (result != KERN_SUCCESS) {
        klog(KLOG_ERROR, "ipc: ipc_object_reference: cannot retain object (%d)\n", result);
    }
}

kern_return_t ipc_object_try_reference(ipc_object_t *object) {
    if (!IO_VALID(object)) {
        return KERN_INVALID_ARGUMENT;
    }

    io_lock(object);

    if (object->ref_count == UINT32_MAX || !io_active(object)) {
        io_unlock(object);
        return KERN_ABORTED;
    }

    io_reference(object);
    io_unlock(object);
    return KERN_SUCCESS;
}

void ipc_object_release(ipc_object_t *object) {
    // Check for valid object before attempting to release it.
    if (!IO_VALID(object)) {
        klog(KLOG_ERROR, "ipc: ipc_object_release: invalid object\n");
        return;
    }

    io_lock(object);
    if (object->ref_count == 0) {
        io_unlock(object);
        klog(KLOG_ERROR, "ipc: ipc_object_release: reference count underflow\n");
        return;
    }
    io_release(object);
    bool last_reference = object->ref_count == 0;
    if (last_reference) {
        object->bits &= ~IO_BITS_ACTIVE;
    }
    io_unlock(object);

    if (last_reference) {
        if (io_otype(object) == IO_TYPE_PORT) {
            ipc_port_dealloc((ipc_port_t *)object);
        } else {
            kfree(object);
        }
    }
}