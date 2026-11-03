#ifndef CELESTEOS_SPINLOCK_H
#define CELESTEOS_SPINLOCK_H

#include <stdint.h>

typedef struct KeSpinLock {
    volatile uint32_t lock;
} KeSpinLock;

extern void KeSpinLockInit(KeSpinLock *lock);
extern void KeSpinLockAcquire(KeSpinLock *lock);
extern int KeSpinLockTryAcquire(KeSpinLock *lock);
extern void KeSpinLockRelease(KeSpinLock *lock);
extern uint64_t KeSpinLockAcquireIrqSave(KeSpinLock *lock);
extern void KeSpinLockReleaseIrqRestore(KeSpinLock *lock, uint64_t flags);

#endif // CELESTEOS_SPINLOCK_H