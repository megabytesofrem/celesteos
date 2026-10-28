/**
 * File: task.c
 *
 * Purpose: A task in the scheduler of CelesteOS.
 */
#include "task.h"

task_t g_tasks[MAX_TASKS];
kobject_pool_t g_task_pool;
task_t *g_current_task;

#define HANDLE_INDEX_BITS 8
#define HANDLE_INDEX_MASK ((UINT64_C(1) << HANDLE_INDEX_BITS) - 1)
#define HANDLE_GENERATION_MASK ((uint64_t)INT64_MAX >> HANDLE_INDEX_BITS)

/* Forward declarations */
static void task_destroy(kobject_t *obj);

static const kobject_ops_t g_task_ops = { .name = "task", .destroy = task_destroy };

/**
 * Encode a handle from the given index and handle entry.
 *
 * Bits 0-7 hold the index, and the remaining bits hold the generation number.
 */
static handle_t task_encode_handle(size_t index, const handle_entry_t *entry) {
    // Encode the handle with the generation and index.
    uint64_t value = (entry->generation << HANDLE_INDEX_BITS) | index;
    return (handle_t)value;
}

/**
 * Decode a handle into its index and generation number.
 *
 * Bits 0-7 hold the index, and the remaining bits hold the generation number.
 */
static bool task_decode_handle(handle_t handle, size_t *index, uint64_t *generation) {
    if (handle < 0) {
        return false;
    }

    uint64_t value = (uint64_t)handle;

    // Decode the handle into the index and generation.

    // For example, handle "slot 5, generation 7" refers to slot 5's current object. If
    // that handle is closed and slot 5 is reused, the kernel will increment the generation number
    // for that slot.
    *index = value & HANDLE_INDEX_MASK;
    *generation = value >> HANDLE_INDEX_BITS;
    return *index < MAX_HANDLES_PER_TASK;
}

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
        handle_t handle = task_encode_handle((size_t)i, &task->handles[i]);
        task_release_handle(task, handle);
    }
}

task_t *task_create(void) {
    // Allocate a new kobject from the task pool
    task_t *task = (task_t *)kobject_alloc(&g_task_pool, KOBJ_TYPE_TASK);
    if (!task) {
        return NULL;
    }

    spinlock_init(&task->handle_lock);
    task->pid = 0;
    task->cr3_page_table = NULL;
    task->thread_count = 0;
    for (int i = 0; i < MAX_HANDLES_PER_TASK; i++) {
        task->handles[i].kobj = NULL;
        task->handles[i].rights = 0;
    }

    return task;
}

task_t *task_current(void) {
    return g_current_task;
}

handle_t task_insert_handle(task_t *task, kobject_t *kobj, uint32_t rights) {
    if (!task || !kobj) {
        return INVALID_HANDLE;
    }

    spinlock_acquire(&task->handle_lock);
    for (int i = 0; i < MAX_HANDLES_PER_TASK; i++) {
        handle_entry_t *entry = &task->handles[i];
        if (!entry->kobj) {
            kobject_retain(kobj);
            entry->kobj = kobj;
            entry->rights = rights;
            handle_t handle = task_encode_handle((size_t)i, entry);
            spinlock_release(&task->handle_lock);
            return handle;
        }
    }
    spinlock_release(&task->handle_lock);
    return INVALID_HANDLE;
}

kobject_t *task_lookup_handle(task_t *task, handle_t handle, uint32_t required_rights) {
    size_t index;
    uint64_t generation;
    if (!task || !task_decode_handle(handle, &index, &generation)) {
        return NULL;
    }

    spinlock_acquire(&task->handle_lock);
    handle_entry_t *entry = &task->handles[index];
    kobject_t *kobj = NULL;

    // Check if the handle is valid by comparing the generation and required rights.
    //
    // This way we enforce capability rights and ensure that stale handles are not used.
    if (entry->kobj && entry->generation == generation &&
        (entry->rights & required_rights) == required_rights) {
        kobj = entry->kobj;
        kobject_retain(kobj);
    }
    spinlock_release(&task->handle_lock);
    return kobj;
}

bool task_release_handle(task_t *task, handle_t handle) {
    size_t index;
    uint64_t generation;

    // Decode the handle into its index and generation components.
    if (!task || !task_decode_handle(handle, &index, &generation)) {
        return false;
    }

    spinlock_acquire(&task->handle_lock);
    handle_entry_t *entry = &task->handles[index];

    // Check if the handle is valid by comparing the generation.
    //
    // If the generation does not match, the handle is stale and cannot be released.
    if (!entry->kobj || entry->generation != generation) {
        spinlock_release(&task->handle_lock);
        return false;
    }

    kobject_t *kobj = entry->kobj;
    entry->kobj = NULL;
    entry->rights = 0;
    entry->generation = (entry->generation + 1) & HANDLE_GENERATION_MASK;
    spinlock_release(&task->handle_lock);

    kobject_release(kobj);
    return true;
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