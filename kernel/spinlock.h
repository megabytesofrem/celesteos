#ifndef CELESTEOS_SPINLOCK_H
#define CELESTEOS_SPINLOCK_H

#include <stdint.h>

typedef struct {
    volatile uint32_t lock;
} spinlock_t;

extern void spinlock_init(spinlock_t *lock);
extern void spinlock_acquire(spinlock_t *lock);
extern int spinlock_try_acquire(spinlock_t *lock);
extern void spinlock_release(spinlock_t *lock);
extern uint64_t spinlock_acquire_irqsave(spinlock_t *lock);
extern void spinlock_release_irqrestore(spinlock_t *lock, uint64_t flags);

#endif // CELESTEOS_SPINLOCK_H