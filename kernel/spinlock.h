#ifndef CELESTEOS_SPINLOCK_H
#define CELESTEOS_SPINLOCK_H

#include <stdint.h>

typedef struct {
    volatile uint32_t lock;
} spinlock_t;

static inline void spinlock_init(spinlock_t *lock) {
    lock->lock = 0;
}

static inline void spinlock_acquire(spinlock_t *lock) {
    while (__atomic_test_and_set(&lock->lock, __ATOMIC_ACQUIRE)) {
        // Instruct the CPU that we are in a spin-wait loop to improve performance
        __asm__ volatile("pause" ::: "memory");
    }
}

static inline int spinlock_try_acquire(spinlock_t *lock) {
    return !__atomic_test_and_set(&lock->lock, __ATOMIC_ACQUIRE);
}

static inline void spinlock_release(spinlock_t *lock) {
    __atomic_clear(&lock->lock, __ATOMIC_RELEASE);
}

#endif // CELESTEOS_SPINLOCK_H