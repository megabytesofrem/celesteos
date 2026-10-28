#ifndef CELESTEOS_TASK_H
#define CELESTEOS_TASK_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "kobject.h"
#include <kernel/svc.h>

#define MAX_HANDLES_PER_TASK 256
#define MAX_TASKS 1024

#define CAP_RIGHT_READ (1 << 0)
#define CAP_RIGHT_WRITE (1 << 1)
#define CAP_RIGHT_CALL (1 << 2)
#define CAP_RIGHT_GRANT (1 << 3)

typedef struct handle_entry {
    kobject_t *kobj;

    /* Hold the capability rights for this handle entry */
    uint32_t rights;

    /*
     * Hold the generation number for this handle entry.
     * Each time the handle is released and the slot is reused, we increment it.
     */
    uint64_t generation;
} handle_entry_t;

typedef struct task {
    kobject_t kobj;
    spinlock_t handle_lock;

    /* Hold the process ID for this task */
    uint64_t pid;

    /* HHDM virtual address of the task's PML4 */
    struct page_table *cr3_page_table;

    struct handle_entry handles[MAX_HANDLES_PER_TASK];
    uint64_t thread_count;
} task_t;

/* Global task array, task pool, and current task */
extern task_t g_tasks[MAX_TASKS];
extern kobject_pool_t g_task_pool;
extern task_t *g_current_task;

/**
 * Initialize the task subsystem.
 *
 * This function should be called early on in the kernel to setup
 * the task subsystem.
 */
void task_sys_init(void);

/**
 * Create a new task, returning a pointer to the newly created task.
 */
task_t *task_create(void);

/**
 * Return the currently executing task.
 */
task_t *task_current(void);

/**
 * Insert a new handle into the task's handle table with the specified rights.
 *
 * The task takes a reference to kobj. Returns INVALID_HANDLE if insertion fails.
 */
handle_t task_insert_handle(task_t *task, kobject_t *kobj, uint32_t rights);

/**
 * Look up a handle, checking that it is valid and grants all required rights.
 * The returned kobject has a retained reference; the caller must release it.
 */
kobject_t *task_lookup_handle(task_t *task, handle_t handle, uint32_t required_rights);

/**
 * Release a handle from the task's handle table.
 * Returns true if the handle was valid and released.
 */
bool task_release_handle(task_t *task, handle_t handle);

/**
 * Attach a thread to the specified task.
 */
void task_attach_thread(task_t *task, void *thread);

/**
 * Detach a thread from the specified task.
 */
void task_detach_thread(task_t *task, void *thread);

#endif // CELESTEOS_TASK_H