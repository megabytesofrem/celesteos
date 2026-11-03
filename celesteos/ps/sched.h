#ifndef CELESTEOS_SCHED_H
#define CELESTEOS_SCHED_H

#include <ps/psthread.h>

#include <stdint.h>

/**
 * Initializes the CPU core scheduler infrastructure and sets up the initial runqueue.
 */
void PsSchedulerInit(void);

/**
 * Returns a pointer to the TCB of the currently executing thread.
 */
PsThread *PsSchedulerCurrentThread(void);

/**
 * Enqueues a thread into the READY state, making it eligible to run.
 */
void PsSchedulerEnqueueThread(PsThread *thread);

/**
 * Removes a thread from the active runqueue (e.g., when blocking on IPC).
 */
void PsSchedulerDequeueThread(PsThread *thread);

/**
 * Triggers a scheduling pass. Picks the next READY thread and performs
 * a context switch if the thread context changes.
 */
void PsSchedulerSchedule(void);

/**
 * Yields the CPU from the current thread, allowing the scheduler to pick the next
 * READY thread.
 */
void PsSchedulerYield(void);

/**
 * Switch directly to the specified target thread, bypassing the ready queue.
 */
void PsSchedulerSwitchTo(PsThread *thread);

/**
 * Low-level assembly context switch interface implemented in ctx_switch.s.
 * Saves current callee-saved registers, updates old_rsp, loads new_rsp,
 * and restores saved state into CPU.
 */
extern void ctx_switch(uint64_t **oldRsp, uint64_t *newRsp);

#endif // CELESTEOS_SCHED_H
