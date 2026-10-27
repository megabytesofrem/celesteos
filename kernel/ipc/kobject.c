#include "kobject.h"

void kobject_pool_init(kobject_pool_t *pool, void *base, size_t stride, size_t count,
                       const kobject_ops_t *ops) {
    if (!pool) {
        return;
    }

    pool->base = base;
    pool->stride = stride;
    pool->count = count;
    pool->free_head = NULL;
    pool->used = 0;
    pool->peak = 0;
    pool->ops = ops;

    spinlock_init(&pool->lock);
}

kobject_t *kobject_alloc(kobject_pool_t *pool, kobject_type_t type) {
    if (!pool || !pool->base || !pool->ops) {
        return NULL;
    }

    // Acquire a lock on the pool before allocating an object
    spinlock_acquire(&pool->lock);

    kobject_t *obj = NULL;
    if (pool->free_head) {
        // Allocate from the free list first, if we can reuse an existing object
        obj = pool->free_head;

        // Remove the object from the free list
        pool->free_head = obj->free_next;
    } else if (pool->used < pool->count) {
        // Allocate from the static pool if there are available slots
        obj = (kobject_t *)(pool->base + (pool->used * pool->stride));

        // Increment the used count as we are allocating a new object from the pool
        pool->used++;
        if (pool->used > pool->peak) {
            pool->peak = pool->used;
        }
    }

    if (obj) {
        obj->type = type;
        obj->ref_count = 1;
        obj->pool = pool;
        obj->free_next = NULL;
    }

    spinlock_release(&pool->lock);

    return obj;
}

kobject_t *kobject_construct(kobject_pool_t *pool, kobject_type_t type, kobject_data_t data) {
    kobject_t *obj = kobject_alloc(pool, type);

    // Call the constructor for the object if it exists
    if (obj && pool->ops && pool->ops->construct) {
        pool->ops->construct(obj, data);
    }
    return obj;
}

void kobject_retain(kobject_t *obj) {
    if (!obj) {
        return;
    }

    __atomic_fetch_add(&obj->ref_count, 1, __ATOMIC_RELAXED);
}

bool kobject_release(kobject_t *obj) {
    if (!obj) {
        return false;
    }

    if (__atomic_fetch_sub(&obj->ref_count, 1, __ATOMIC_RELEASE) == 1) {
        __atomic_thread_fence(__ATOMIC_ACQUIRE);

        // Destroy the object as its last reference has been released
        kobject_destroy(obj);
        return true;
    }

    return false;
}

void kobject_destroy(kobject_t *obj) {
    if (!obj || !obj->pool) {
        return;
    }

    kobject_pool_t *pool = obj->pool;

    // Invoke the destructor if it exists
    if (pool->ops && pool->ops->destroy) {
        pool->ops->destroy(obj);
    }

    // Aquire a lock on the pool before modifying the free list
    spinlock_acquire(&pool->lock);

    obj->type = KOBJ_TYPE_NONE;
    obj->free_next = pool->free_head;
    pool->free_head = obj;

    spinlock_release(&pool->lock);
}