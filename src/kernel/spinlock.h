#ifndef CELESTEOS_SPINLOCK_H
#define CELESTEOS_SPINLOCK_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    volatile uint32_t lock;
} spinlock_t;

static inline void spinlock_init(spinlock_t* lock) {
    lock->lock = 0;
}

static inline void spinlock_acquire(spinlock_t* lock) {
    // Spin until the lock is acquired
    while (__atomic_test_and_set(&lock->lock, __ATOMIC_ACQUIRE)) {
        __builtin_ia32_pause();
    }
}

static inline bool spinlock_try_acquire(spinlock_t* lock) {
    // Try to acquire the lock
    return !__atomic_test_and_set(&lock->lock, __ATOMIC_ACQUIRE);
}

static inline void spinlock_release(spinlock_t* lock) {
    __atomic_clear(&lock->lock, __ATOMIC_RELEASE);
}

#endif  // CELESTEOS_SPINLOCK_H