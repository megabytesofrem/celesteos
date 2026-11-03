#include <ob/obcreation.h>

#include <stdatomic.h>
#include <string.h>

void ObObjectPoolInit(ObObjectPool *pool, void *backingStore, size_t objectSize, size_t objectCount,
    const ObOperations *ops) {
    if (!pool) {
        return;
    }

    pool->backingStore = backingStore;
    pool->objectSize = objectSize;
    pool->objectCount = objectCount;
    pool->nextIndex = 0;
    pool->usedCount = 0;
    pool->peakUsed = 0;
    pool->freeHead = NULL;
    pool->ops = ops;
    KeSpinLockInit(&pool->lock);
}

ObHeader *ObAllocInPool(ObObjectPool *pool, ObObjectType type) {
    if (!pool || !pool->backingStore || pool->objectSize < sizeof(ObHeader) || !pool->ops) {
        return NULL;
    }

    KeSpinLockAcquire(&pool->lock);

    ObHeader *object = pool->freeHead;
    if (object) {
        pool->freeHead = object->nextFree;
    } else if (pool->nextIndex < pool->objectCount) {
        object = (ObHeader *)((uint8_t *)pool->backingStore + pool->nextIndex * pool->objectSize);
        ++pool->nextIndex;
    }

    if (object) {
        memset(object, 0, pool->objectSize);
        object->type = type;
        atomic_store_explicit(&object->references, 1, memory_order_relaxed);
        object->pool = pool;
        ++pool->usedCount;
        if (pool->usedCount > pool->peakUsed) {
            pool->peakUsed = pool->usedCount;
        }
    }

    KeSpinLockRelease(&pool->lock);
    return object;
}

ObHeader *ObCreateInPool(ObObjectPool *pool, ObObjectType type, ObObjectData data) {
    ObHeader *object = ObAllocInPool(pool, type);
    if (object && pool->ops->create) {
        pool->ops->create(object, data);
    }
    return object;
}

void ObRetain(ObHeader *object) {
    if (!object) {
        return;
    }

    uint32_t references = atomic_load_explicit(&object->references, memory_order_relaxed);
    while (references != 0 && references != UINT32_MAX &&
           !atomic_compare_exchange_weak_explicit(&object->references, &references, references + 1,
               memory_order_relaxed, memory_order_relaxed)) {
    }
}

bool ObRelease(ObHeader *object) {
    if (!object) {
        return false;
    }

    uint32_t references = atomic_load_explicit(&object->references, memory_order_relaxed);
    while (references != 0) {
        if (atomic_compare_exchange_weak_explicit(&object->references, &references, references - 1,
                memory_order_release, memory_order_relaxed)) {
            if (references != 1) {
                return false;
            }

            atomic_thread_fence(memory_order_acquire);
            ObObjectPool *pool = object->pool;
            if (!pool) {
                return true;
            }

            if (pool->ops && pool->ops->destroy) {
                pool->ops->destroy(object);
            }

            KeSpinLockAcquire(&pool->lock);
            object->type = ObObjectTypeNone;
            object->nextFree = pool->freeHead;
            pool->freeHead = object;
            --pool->usedCount;
            KeSpinLockRelease(&pool->lock);
            return true;
        }
    }
    return false;
}

void ObDestroy(ObHeader *object) {
    (void)ObRelease(object);
}
