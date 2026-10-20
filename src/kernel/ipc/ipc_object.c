#include "ipc_object.h"

#include <kernel/kmalloc.h>

kern_return_t ipc_object_alloc(ipc_space_t *space, ipc_object_type_t type, mach_port_type_t ptype,
                               mach_port_urefs_t refs, mach_port_name_t *name,
                               ipc_object_t **object) {
    STUB("ipc_object_alloc: not implemented");
}

kern_return_t ipc_object_alloc_name(ipc_space_t *space, ipc_object_type_t type,
                                    mach_port_type_t ptype, mach_port_urefs_t refs,
                                    mach_port_name_t name, ipc_object_t **object) {
    STUB("ipc_object_alloc_name: not implemented");
}

void ipc_object_destroy(ipc_object_t *object, mach_msg_type_name_t msg_type) {
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