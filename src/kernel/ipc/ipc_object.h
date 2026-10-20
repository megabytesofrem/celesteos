/**
 * File: ipc/ipc_object.h
 *
 * IPC object definitions and declarations.
 *
 * Sources: https://github.com/flavioc/gnumach/blob/master/ipc/ipc_object.h
 */

#ifndef CELESTEOS_IPC_OBJECT_H
#define CELESTEOS_IPC_OBJECT_H

#include <ipc/mach_message.h>
#include <kernel/spinlock.h>
#include <mach/kern_return.h>
#include <mach/mach_types.h>

/* Reference count for objects*/
typedef natural_t ipc_object_ref_count_t;

typedef natural_t ipc_object_bits_t;
typedef natural_t ipc_object_type_t;

/* Forward declaration */
typedef struct ipc_space ipc_space_t;

typedef struct ipc_object {
    spinlock_t lock;
    ipc_object_ref_count_t ref_count;
    ipc_object_bits_t bits;
} ipc_object_t;

/* State of IPC objects: either dead, null, or valid (alive) */
#define IO_NULL ((ipc_object_t *)0)
#define IO_DEAD ((ipc_object_t *)UINTPTR_MAX)
#define IO_VALID(io) (((io) != IO_NULL) && ((io) != IO_DEAD))

/* Types and bit masks*/
#define IO_BITS_KOTYPE 0x0000ffff    /* kernel object type */
#define IO_BITS_OTYPE 0x3fff0000     /* object type */
#define IO_BITS_PROTECTED 0x40000000 /* protected payload bit */
#define IO_BITS_ACTIVE 0x80000000U   /* active object bit */

/**
 * Accessors for IPC object bitfields.
 */
#define io_active(io) (((io)->bits & IO_BITS_ACTIVE) != 0)
#define io_otype(io) (((io)->bits & IO_BITS_OTYPE) >> 16)
#define io_kotype(io) ((io)->bits & IO_BITS_KOTYPE)

/* Construct the bitfield for an IPC object */
#define io_makebits(is_active, obj_type, kernel_type)                                              \
    ((ipc_object_bits_t)(((is_active) ? IO_BITS_ACTIVE : 0u) |                                     \
                         (((ipc_object_bits_t)(obj_type) << 16) & IO_BITS_OTYPE) |                 \
                         ((ipc_object_bits_t)(kernel_type) & IO_BITS_KOTYPE)))

/**
 * IPC object types (ports, port sets, kernel loaded ports)
 */
#define IO_TYPE_PORT 0
#define IO_TYPE_PORT_SET 1
#define IO_TYPE_NUMBER 2

/* Wrappers around kernel/spinlock.c */
#define io_lock_init(io) spinlock_init(&(io)->lock)
#define io_lock(io) spinlock_acquire(&(io)->lock)
#define io_trylock(io) spinlock_try_acquire(&(io)->lock)
#define io_unlock(io) spinlock_release(&(io)->lock)

/* Caller holds the object lock when changing references directly. */
#define io_reference(io) ((io)->ref_count++)
#define io_release(io) ((io)->ref_count--)

/**
 * Allocate an IPC object.
 */
kern_return_t ipc_object_alloc(ipc_space_t *space, ipc_object_type_t type, mach_port_type_t ptype,
                               mach_port_urefs_t refs, mach_port_name_t *name,
                               ipc_object_t **object);

/**
 * Allocate an IPC object with a specific name.
 */
kern_return_t ipc_object_alloc_name(ipc_space_t *space, ipc_object_type_t type,
                                    mach_port_type_t ptype, mach_port_urefs_t refs,
                                    mach_port_name_t name, ipc_object_t **object);

/**
 * Destroy an IPC object, cleaning up any associated resources.
 */
void ipc_object_destroy(ipc_object_t *object, mach_msg_type_name_t msg_type);

/**
 * Increment the reference count of an IPC object.
 */
void ipc_object_reference(ipc_object_t *object);

/**
 * Decrement the reference count and free the object on its last reference.
 * Type-specific resources must already be detached before the last release.
 */
void ipc_object_release(ipc_object_t *object);

#endif // CELESTEOS_IPC_OBJECT_H