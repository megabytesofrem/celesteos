#include "ipc_object.h"

#include <kernel/kmalloc.h>

kern_return_t ipc_object_alloc(ipc_space_t *space, ipc_object_type_t type, mach_port_type_t ptype,
                               mach_port_urefs_t refs, mach_port_name_t *name,
                               ipc_object_t **object) {

    /**
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
     * Psuedocode:
     *
     * kern_return_t ipc_object_alloc(...) {
     *      validate arguments and capabilities
     *      clear outputs
     *
     *      allocate the object and set up initial capability
     *      pepare any storage needed for the entry
     *      lock space
     *          install capability metadata and object pointer
     *          establish the entry in the space and its linkage to the object
     *      unlock space
     *
     *      return success or failure, with object pointer set on success
     *      on failure:
     *          release all private resources allocated for the object
     *          set the object pointer to NULL
     *          return an error code
     * }
     */

    STUB("ipc_object_alloc: not implemented");
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

    STUB("ipc_object_alloc_name: not implemented");
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

    // Check for valid object before attempting to destroy it.
    if (!IO_VALID(object)) {
        klog(KLOG_ERROR, "ipc: ipc_object_destroy: invalid object\n");
        return;
    }

    STUB_NORETURN("ipc_object_destroy: not implemented, STUB kfree(object)");

    kfree(object);
}

void ipc_object_reference(ipc_object_t *object) {
    // Check for valid object before attempting to reference it.
    if (!IO_VALID(object)) {
        klog(KLOG_ERROR, "ipc: ipc_object_reference: invalid object\n");
        return;
    }

    io_lock(object);

    // Check for invalid reference count before incrementing
    if (object->ref_count == UINT32_MAX) {
        io_unlock(object);
        klog(KLOG_ERROR, "ipc: ipc_object_reference: invalid reference count\n");
        return;
    }

    io_reference(object);
    io_unlock(object);
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
        kfree(object);
    }
}