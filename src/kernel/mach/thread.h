#ifndef CELESTEOS_MACH_THREAD_H
#define CELESTEOS_MACH_THREAD_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include <kernel/spinlock.h>

#include "port.h"

/* Forward declarations for kernel thread-related structures */
typedef struct task task_t;
struct mach_kmsg;
struct mach_port_space;

typedef enum {
    THREAD_RUNNING,
    THREAD_WAITING, /* waiting for IPC */
    THREAD_DEAD     /* terminated */
} thread_state_t;

/**
 * Saved CPU registers for a thread's context.
 */
typedef struct cpu_context {
    uint64_t r15, r14, r13, r12; /* callee-saved registers */
    uint64_t rbp, rbx;           /* base pointer and base register */
    uint64_t rip;                /* instruction pointer */
    uint64_t rsp;                /* stack pointer */
    uint64_t rflags;             /* flags register */
} cpu_context_t;

/**
 * A Mach thread structure.
 * 
 * Mach threads are lightweight execution units within a task.
 */
typedef struct thread {
    spinlock_t lock; /* lock for synchronizing access to the thread structure */
    uint32_t tid;
    thread_state_t state; /* current scheduler state */

    /* CPU execution context */
    cpu_context_t context;
    uintptr_t kstack_base; /* base address of the kernel stack */
    uintptr_t kstack_top;  /* top address of the kernel stack */

    /* Parent task */
    task_t* parent_task; /* pointer to the parent task */
    struct thread* next; /* pointer to the next thread in the task's thread list */

    /* IPC state*/
    ipc_port_t* waiting_on_port; /* the port the thread is waiting on */
    int32_t wait_result;         /* result of the wait operation */
} thread_t;

/**
 * Create a new thread attached to a parent task.
 * 
 * Allocates thread_t, initializes its context and kernel stack, and links it to the parent task.
 */
thread_t* thread_create(task_t* parent, uintptr_t entry_point);

/**
 * Destroy a thread and free its resources.
 *
 * Removes the thread from its parent task's thread list, deallocates its kernel stack, and frees the thread structure.
 */
void thread_destroy(thread_t* thread);

/**
 * Block a thread on a specific IPC port.
 *
 * Sets the thread's waiting_on_port to the specified port and changes its state to THREAD_WAITING.
 */
void thread_blockon(thread_t* thread, ipc_port_t* port);

/**
 * Unblock a thread that was previously waiting on an IPC port, and places it back into the run queue.
 */
void thread_unblock(thread_t* thread, int32_t result);

#endif  // CELESTEOS_MACH_THREAD_H