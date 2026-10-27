/**
 * File: thread.c
 *
 * Purpose: A thread in the scheduler of CelesteOS. Threads represent an execution context
 * within a task.
 */

#include "thread.h"
#include "kobject.h"
#include "scheduler.h"
#include "task.h"

#include <kernel/tss.h>
#include <string.h>

thread_t g_threads[MAX_THREADS];
kobject_pool_t g_thread_pool;

_Atomic thread_id_t current_thread_id = 0;

/* Forward declarations */
static void thread_start(void) __attribute__((noreturn));
static void thread_destroy(kobject_t *obj);

static const kobject_ops_t g_thread_ops = {
    .name = "thread",
    .destroy = thread_destroy,
};

thread_id_t next_thread_id(void) {
    return __atomic_add_fetch(&current_thread_id, 1, __ATOMIC_SEQ_CST);
}

void thread_sys_init(void) {
    kobject_pool_init(&g_thread_pool, g_threads, sizeof(thread_t), MAX_THREADS, &g_thread_ops);
}

static void thread_start(void) {
    // Get the currently running thread from the scheduler
    thread_t *thread = scheduler_get_current();

    if (!thread) {
        __builtin_trap();
    }

    // Switch to user mode with the threads saved context (RIP, RSP, RFLAGS)
    tss_switch_ring3(thread->rip, thread->user_rsp);
    __builtin_unreachable();
}

static void thread_destroy(kobject_t *obj) {
    thread_t *thread = (thread_t *)obj;

    /* Object is dead (ref count = 0); zero out fields without acquiring spinlock */
    thread->tid = -1;
    thread->state = THREAD_INACTIVE;
    thread->parent_task = NULL;
    thread->rsp = NULL;
    thread->next = NULL;
    thread->prev = NULL;
}

thread_t *thread_create(struct task *parent_task, void *entry_point, void *user_stack_top) {
    if (!parent_task || !parent_task->cr3_page_table || !entry_point || !user_stack_top) {
        return NULL;
    }

    // Allocate a new kobject from the thread pool
    kobject_t *obj = kobject_alloc(&g_thread_pool, KOBJ_TYPE_THREAD);
    if (!obj) {
        return NULL;
    }

    thread_t *thread = (thread_t *)obj;
    spinlock_init(&thread->lock);

    // Acquire the thread lock to initialize its fields safely
    spinlock_acquire(&thread->lock);

    // Initialize thread fields
    thread->tid = next_thread_id();
    thread->state = THREAD_INACTIVE;
    thread->parent_task = parent_task;
    thread->rip = (uintptr_t)entry_point;
    thread->user_rsp = (uintptr_t)user_stack_top;
    thread->rflags = 0x202; /* IF = 1 */
    thread->ipc_partner_tid = -1;
    memset(thread->ipc_msg_payload, 0, sizeof(thread->ipc_msg_payload));
    thread->next = NULL;
    thread->prev = NULL;

    // Setup initial kernel stack frame for the new thread
    uintptr_t stack_top = (uintptr_t)thread->kernel_stack + sizeof(thread->kernel_stack);
    stack_top &= ~(uintptr_t)0xF; /* 16-byte ABI alignment */

    uintptr_t *frame = (uintptr_t *)(stack_top - (7 * sizeof(uintptr_t)));
    frame[0] = 0;                       /* r15 */
    frame[1] = 0;                       /* r14 */
    frame[2] = 0;                       /* r13 */
    frame[3] = 0;                       /* r12 */
    frame[4] = 0;                       /* rbx */
    frame[5] = 0;                       /* rbp */
    frame[6] = (uintptr_t)thread_start; /* return address */

    thread->rsp = (uint64_t *)frame;

    // Release lock after initializing the thread's fields
    spinlock_release(&thread->lock);
    return thread;
}