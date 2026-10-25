#ifndef CELESTEOS_SCHEDULER_H
#define CELESTEOS_SCHEDULER_H

#include "thread.h"
#include <stddef.h>
#include <stdint.h>

/**
 * Initializes the CPU core scheduler infrastructure and sets up the initial runqueue.
 */
void scheduler_init(void);

/**
 * Returns a pointer to the TCB of the currently executing thread.
 */
thread_t *scheduler_get_current(void);

/**
 * Enqueues a thread into the READY state, making it eligible to run.
 */
void scheduler_enqueue(thread_t *thread);

/**
 * Removes a thread from the active runqueue (e.g., when blocking on IPC).
 */
void scheduler_dequeue(thread_t *thread);

/**
 * Triggers a scheduling pass. Picks the next READY thread and performs
 * a context switch if the thread context changes.
 */
void schedule(void);

void perform_switch(thread_t *prev, thread_t *next);

/**
 * Low-level assembly context switch interface implemented in ctx_switch.s.
 * Saves current callee-saved registers, updates old_rsp, loads new_rsp,
 * and restores saved state into CPU.
 */
extern void ctx_switch(uint64_t **old_rsp, uint64_t *new_rsp);

#endif // CELESTEOS_SCHEDULER_H