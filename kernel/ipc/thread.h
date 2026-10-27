#ifndef CELESTEOS_THREAD_H
#define CELESTEOS_THREAD_H

#include "kobject.h"

#include <stddef.h>
#include <stdint.h>

/* Maximum number of threads alive at any given time */
#define MAX_THREADS 1024

typedef struct thread thread_t;
typedef int64_t thread_id_t;
typedef int64_t task_id_t;

typedef enum {
    THREAD_INACTIVE,
    THREAD_RUNNING,
    THREAD_READY,
    THREAD_BLOCKED_SEND,
    THREAD_BLOCKED_RECV
} thread_state_t;

typedef struct thread {
    /* Kernel object header */
    kobject_t kobj;
    spinlock_t lock;

    thread_id_t tid;
    thread_state_t state;

    uint64_t *rsp;
    uint64_t rip;
    uint64_t rflags;
    uintptr_t user_rsp;

    /* Kernel stack for this thread */
    uint8_t kernel_stack[8192] __attribute__((aligned(16)));

    /* Parent task of this thread */
    struct task *parent_task;

    thread_id_t ipc_partner_tid;
    uint64_t ipc_msg_payload[6];

    struct thread *next;
    struct thread *prev;
} thread_t;

/* Global thread array and thread pool (needs the complete thread_t above) */
extern thread_t g_threads[MAX_THREADS];
extern kobject_pool_t g_thread_pool;

thread_id_t next_thread_id(void);

/**
 * Initialize the thread subsystem.
 *
 * This function should be called early on in the kernel to setup the
 * thread management subsystem.
 */
void thread_sys_init(void);

/**
 * Create a new thread within the specified parent task.
 *
 * The new thread is not runnable until it is passed to scheduler_enqueue().
 * The caller owns the returned reference and drops it with kobject_release().
 */
thread_t *thread_create(struct task *parent_task, void *entry_point, void *user_stack_top);

#endif // CELESTEOS_THREAD_H