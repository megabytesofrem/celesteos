/**
 * File: task.c
 *
 * Purpose: A task in the scheduler of CelesteOS.
 */
#include "task.h"

task_t g_tasks[MAX_TASKS];
kobject_pool_t g_task_pool;
task_t *g_current_task;

/* Forward declarations */
static void task_destroy(kobject_t *obj);

static const kobject_ops_t g_task_ops = { .name = "task", .destroy = task_destroy };

void task_sys_init(void) {
    g_current_task = NULL;

    kobject_pool_init(&g_task_pool, g_tasks, sizeof(task_t), MAX_TASKS, &g_task_ops);
}

static void task_destroy(kobject_t *obj) {
    task_t *task = (task_t *)obj;

    task->pid = 0;
    task->cr3_page_table = NULL;
    task->thread_count = 0;

    // Clear all handle entries for the task.
    for (int i = 0; i < MAX_HANDLES_PER_TASK; i++) {
        task_release_handle(task, &task->handles[i]);
    }
}

task_t *task_create(void) {
    // Allocate a new kobject from the task pool
    task_t *task = (task_t *)kobject_alloc(&g_task_pool, KOBJ_TYPE_TASK);
    if (!task) {
        return NULL;
    }

    task->pid = 0;
    task->cr3_page_table = NULL;
    task->thread_count = 0;
    for (int i = 0; i < MAX_HANDLES_PER_TASK; i++) {
        task->handles[i].kobj = NULL;
        task->handles[i].rights = 0;
        task->handles[i].target = NULL;
    }

    return task;
}

task_t *task_current(void) {
    return g_current_task;
}

handle_entry_t *task_insert_handle(task_t *task, kobject_t *kobj, uint32_t rights) {
    // Find the first available slot to insert the handle
    for (int i = 0; i < MAX_HANDLES_PER_TASK; i++) {
        if (!task->handles[i].kobj) {
            task->handles[i].kobj = kobj;
            task->handles[i].rights = rights;
            task->handles[i].target = NULL;
            return &task->handles[i];
        }
    }
    return NULL;
}

kobject_t *task_lookup_handle(task_t *task, handle_entry_t *handle) {
    if (!handle) {
        return NULL;
    }

    // Look for the handle in the task's handle array
    for (int i = 0; i < MAX_HANDLES_PER_TASK; i++) {
        if (&task->handles[i] == handle) {
            // Found the handle, return its associated kobject
            return task->handles[i].kobj;
        }
    }
    return NULL;
}

void task_release_handle(task_t *task, handle_entry_t *handle) {
    if (!task || !handle) {
        return;
    }

    // Destroy the kobject associated with this handle before releasing it
    kobject_destroy(handle->kobj);

    handle->kobj = NULL;
    handle->rights = 0;
    handle->target = NULL;
}

void task_attach_thread(task_t *task, void *thread) {
    if (!task || !thread) {
        return;
    }

    task->thread_count++;
}

void task_detach_thread(task_t *task, void *thread) {
    if (!task || !thread) {
        return;
    }

    if (task->thread_count > 0) {
        task->thread_count--;
    }
}