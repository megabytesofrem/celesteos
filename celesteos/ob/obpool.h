#ifndef CELESTEOS_OBPOOL_H
#define CELESTEOS_OBPOOL_H

#include <ke/spinlock.h>
#include <ob/obheader.h>

#include <stddef.h>

typedef struct ObObjectPool {
    /* Static backing store for objects */
    void *backingStore;
    size_t objectSize;
    size_t objectCount;
    size_t nextIndex;
    size_t usedCount;
    size_t peakUsed;

    /* Free list and operations for this object pool */
    ObHeader *freeHead;
    const ObOperations *ops;
    KeSpinLock lock;
} ObObjectPool;

/**
 * Initialize a fixed-size pool of objects.
 */
void ObObjectPoolInit(ObObjectPool *pool, void *backingStore, size_t objectSize, size_t objectCount,
    const ObOperations *ops);

#endif // CELESTEOS_OBPOOL_H
