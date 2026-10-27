/**
 * File: kobject.h
 *
 * Purpose: Defines kobjects and a kobject pool for managing allocations.
 */

#ifndef CELESTEOS_KOBJECT_H
#define CELESTEOS_KOBJECT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <kernel/spinlock.h>

typedef enum {
    KOBJ_TYPE_NONE,
    KOBJ_TYPE_THREAD,   /* A thread object */
    KOBJ_TYPE_TASK,     /* A task object */
    KOBJ_TYPE_ENDPOINT, /* An IPC endpoint object */
    KOBJ_TYPE_PAGE,     /* A memory page object */
    KOBJ_TYPE_IRQ,      /* An interrupt request object */
} kobject_type_t;

typedef struct kobject kobject_t;
typedef struct kobject_pool kobject_pool_t;
typedef void *kobject_data_t;

typedef struct kobject_ops {
    const char *name;

    void (*construct)(kobject_t *obj, kobject_data_t data);
    void (*destroy)(kobject_t *obj);
} kobject_ops_t;

typedef struct kobject_pool {
    /* Static backing storage, owned by thread.c/task.c */
    uint8_t *base;
    size_t stride; /* sizeof(thread_t), sizeof(task_t), ... */
    size_t count;  /* Count of objects in the pool */

    /* Allocation free list*/
    struct kobject *free_head; /* Head of the free list */
    size_t used;               /* Number of objects currently allocated */
    size_t peak;               /* Highest number of objects allocated at any point */

    /* Operations for this pool */
    const kobject_ops_t *ops;
    spinlock_t lock;
} kobject_pool_t;

typedef struct kobject {
    kobject_type_t type;
    _Atomic uint32_t ref_count; /* Reference count for the object */
    struct kobject_pool *pool;
    struct kobject *free_next; /* Next object in the free list */
} kobject_t;

void kobject_pool_init(kobject_pool_t *pool, void *base, size_t stride, size_t count,
                       const kobject_ops_t *ops);

/**
 * Allocate a kobject from the specified object pool.
 */
kobject_t *kobject_alloc(kobject_pool_t *pool, kobject_type_t type);

/**
 * Construct a kobject from the specified object pool and initialize it with the provided data.
 *
 * Calls `kobject_alloc` internally, and then the constructor defined on the object.
 */
kobject_t *kobject_construct(kobject_pool_t *pool, kobject_type_t type, kobject_data_t data);

/**
 * Retain a reference to the specified object.
 */
void kobject_retain(kobject_t *obj);

/**
 * Release a reference to the specified object.
 * Returns true if the object was destroyed as a result of this release.
 */
bool kobject_release(kobject_t *obj);

/**
 * Destroy the specified object, returning it to its pool.
 */
void kobject_destroy(kobject_t *obj);
#endif // CELESTEOS_KOBJECT_H