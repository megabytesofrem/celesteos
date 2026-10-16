#ifndef CELESTEOS_MACH_TASK_H
#define CELESTEOS_MACH_TASK_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include <kernel/spinlock.h>
#include <kernel/memory/vm_map.h>
#include <kernel/mach/port_space.h>

/* Task Execution States */
typedef uint32_t task_state_t;
#define TASK_STATE_ACTIVE     0
#define TASK_STATE_SUSPENDED  1
#define TASK_STATE_TERMINATED 2

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
    vm_map_t* vm_map; /* pointer to the task's virtual memory map */

    /* IPC state */
    mach_port_space_t* itk_space; /* pointer to the task's port space */
    mach_port_name_t itk_self;    /* the name of the task's own port */

    /* Threads */
    thread_t* thread_list; /* pointer to the list of threads belonging to the task */
    uint32_t thread_count; /* number of threads belonging to the task */

    /* BSD/POSIX compatibility */
    uint32_t uid; /* stub: user ID for POSIX/BSD */
    uint32_t gid; /* stub: group ID for POSIX/BSD */
} task_t;

/**
 * Create a new Mach task (with an optional parent task).
 */
task_t* task_create(task_t* parent_task);

/**
 * Destroy a Mach task, freeing its memory map and all associated resources.
 */
void task_destroy(task_t* task);

/**
 * Suspend execution of all threads belonging to the specified Mach task.
 */
bool task_suspend(task_t* task);

/**
 * Resume execution of all threads belonging to the specified Mach task.
 */
bool task_resume(task_t* task);

#endif  // CELESTEOS_MACH_TASK_H