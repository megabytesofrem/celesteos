#ifndef CELESTEOS_TASK_H
#define CELESTEOS_TASK_H

#include <stddef.h>
#include <stdint.h>

#include "kobject.h"

#define MAX_HANDLES_PER_TASK 256
#define MAX_TASKS 1024

#define CAP_RIGHT_READ (1 << 0)
#define CAP_RIGHT_WRITE (1 << 1)
#define CAP_RIGHT_CALL (1 << 2)
#define CAP_RIGHT_GRANT (1 << 3)

typedef struct handle_entry {
    kobject_t *kobj;
    uint32_t rights;
    void *target;
} handle_entry_t;

typedef struct task {
    kobject_t kobj;

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
 * Returns a pointer to the newly inserted handle entry, or NULL if the table is full.
 */
handle_entry_t *task_insert_handle(task_t *task, kobject_t *kobj, uint32_t rights);

/**
 * Lookup the kobject associated with a handle in the task's handle table, checking
 * that the handle is valid and the caller has the necessary rights.
 */
kobject_t *task_lookup_handle(task_t *task, handle_entry_t *handle);

/**
 * Release a handle from the task's handle table.
 */
void task_release_handle(task_t *task, handle_entry_t *handle);

/**
 * Attach a thread to the specified task.
 */
void task_attach_thread(task_t *task, void *thread);

/**
 * Detach a thread from the specified task.
 */
void task_detach_thread(task_t *task, void *thread);

#endif // CELESTEOS_TASK_H