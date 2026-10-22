#ifndef CELESTEOS_IPC_TASK_H
#define CELESTEOS_IPC_TASK_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <kernel/ipc/ipc_space.h>
#include <kernel/memory/vm_map.h>
#include <kernel/spinlock.h>

typedef enum {
    TASK_ACTIVE,
    TASK_SUSPENDED,
    TASK_TERMINATED
} task_state_t;

/* Forward declarations for kernel task-related structures */
typedef struct thread thread_t;

/**
 * A Mach task structure.
 *
 * Mach tasks are high-level process containers that represent an isolated address
 * space, thread group, and capability space within the kernel.
 */
typedef struct task {
    spinlock_t lock;    /* lock for synchronizing access to the task structure */
    uint32_t task_id;   /* unique identifier for the task */
    task_state_t state; /* current execution state of the task */

    /* Memory management */
    vm_map_t *vm_map; /* pointer to the task's virtual memory map */

    /* IPC state */
    ipc_space_t *itk_space;    /* pointer to the task's port space */
    mach_port_name_t itk_self; /* the name of the task's own port */

    /* Threads */
    thread_t *thread_list; /* pointer to the list of threads belonging to the task */
    uint32_t thread_count; /* number of threads belonging to the task */

    /* BSD/POSIX compatibility */
    uint32_t uid; /* stub: user ID for POSIX/BSD */
    uint32_t gid; /* stub: group ID for POSIX/BSD */
} task_t;

/**
 * Create a new Mach task (with an optional parent task).
 */
kern_return_t task_create(task_t *parent_task, task_t **out_task);

/**
 * Initialize the bootstrap task and its initial running thread before entering user mode.
 */
kern_return_t task_initialize_kernel(void);

/**
 * Destroy a Mach task, freeing its memory map and all associated resources.
 */
kern_return_t task_destroy(task_t *task);

/**
 * Suspend execution of all threads belonging to the specified Mach task.
 */
kern_return_t task_suspend(task_t *task);

/**
 * Resume execution of all threads belonging to the specified Mach task.
 */
kern_return_t task_resume(task_t *task);

#endif // CELESTEOS_IPC_TASK_H