/**
 * File: scheduler.c
 *
 * Purpose: Implements the thread scheduler for CelesteOS, managing thread
 * execution and context switching.
 */

#include "scheduler.h"
#include "task.h"
#include "wait_queue.h"

#include <stddef.h>
#include <stdint.h>

#include <kernel/tss.h>

/* Pointer to the currently running thread */
static thread_t *g_current_thread;

/* Ready queue for threads */
static wait_queue_t g_thread_queue;

void scheduler_init(void) {
    g_current_thread = NULL;
    wait_queue_init(&g_thread_queue);
}

thread_t *scheduler_get_current(void) {
    return g_current_thread;
}

void scheduler_enqueue(thread_t *thread) {
    if (!thread) {
        return;
    }

    // Enqueue the thread into the ready queue
    thread->state = THREAD_READY;
    thread->next = NULL;
    wait_queue_enqueue(&g_thread_queue, thread);
}

void scheduler_dequeue(thread_t *thread) {
    if (!thread) {
        return;
    }

    // Dequeue the thread from the ready queue if it is currently marked as ready
    if (thread->state == THREAD_READY) {
        wait_queue_dequeue(&g_thread_queue, thread);
    }
    thread->state = THREAD_INACTIVE;
}

void schedule(void) {
    if (wait_queue_is_empty(&g_thread_queue)) {
        return;
    }

    thread_t *prev = g_current_thread;
    thread_t *next = wait_queue_pop(&g_thread_queue);

    // If the previous thread is still running, mark it as ready and re-enqueue it
    if (prev && prev->state == THREAD_RUNNING) {
        prev->state = THREAD_READY;
        wait_queue_enqueue(&g_thread_queue, prev);
    }

    // Perform the context switch to the next thread
    perform_switch(prev, next);
}

void perform_switch(thread_t *prev, thread_t *next) {
    if (!next || prev == next) {
        return;
    }

    // Update the global execution context to the next thread
    g_current_thread = next;
    g_current_thread->state = THREAD_RUNNING;

    // Set the kernel stack for the next thread in the TSS
    tss_set_kernel_stack((uintptr_t)&next->kernel_stack[8192]);

    // Switch address space if the next thread belongs to a different task
    task_t *prev_task = prev ? prev->parent_task : NULL;
    if (next->parent_task != NULL && prev_task != next->parent_task) {
        pmap_switch_pml4(next->parent_task->cr3_page_table);
    }

    // On the first switch there is no previous thread; the boot context's
    // stack pointer is saved into a scratch slot and never resumed.
    static uint64_t *boot_rsp;
    uint64_t **save_rsp = prev ? &prev->rsp : &boot_rsp;

    // Context switch to the next thread
    ctx_switch(save_rsp, next->rsp);
}