#include "thread.h"

#include <string.h>

#include <ipc/task.h>
#include <kernel/kmalloc.h>
#include <mach/kern_return.h>
#include <mach/mach_types.h>

static uintptr_t next_tid = 1;
thread_t *g_current_thread;

/**
 * Generate a unique thread ID.
 *
 * This function is used internally to assign a unique identifier to each thread.
 */
static uintptr_t generate_tid() {
    return next_tid++;
}

kern_return_t thread_create(task_t *parent, uintptr_t entry_point, uintptr_t arg,
                            thread_t **out_thread) {
    if (parent == NULL || out_thread == NULL) {
        return KERN_INVALID_ARGUMENT;
    }
    *out_thread = NULL;
    thread_t *thread = kmalloc(sizeof(thread_t));
    if (thread == NULL) {
        return KERN_NO_MEMORY;
    }
    memset(thread, 0, sizeof(*thread));

    spinlock_init(&thread->lock);
    thread->tid = generate_tid();

    thread->kstack_base = (uintptr_t)kmalloc(KERNEL_STACK_SIZE);
    if (!thread->kstack_base) {
        kfree(thread);
        return KERN_NO_MEMORY;
    }
    thread->kstack_top = thread->kstack_base + KERNEL_STACK_SIZE;

    uintptr_t stack_ptr = thread->kstack_top & ~0xFUL; /* align stack pointer to 16 bytes */

    stack_ptr -= sizeof(cpu_context_t);
    cpu_context_t *cpu_ctx = (cpu_context_t *)stack_ptr;
    memset(cpu_ctx, 0, sizeof(cpu_context_t));

    cpu_ctx->rip = entry_point;
    cpu_ctx->rsp = stack_ptr;
    cpu_ctx->rflags = 0x202; /* default flags with interrupts enabled */
    cpu_ctx->rbx = arg;
    thread->context = cpu_ctx;
    thread->parent_task = parent;
    thread->state = THREAD_STOPPED;
    thread->next = parent->thread_list;
    parent->thread_list = thread;
    parent->thread_count++;

    *out_thread = thread;
    return KERN_SUCCESS;
}

kern_return_t thread_destroy(thread_t *thread) {
    if (thread == NULL) {
        return KERN_INVALID_ARGUMENT;
    }
    if (thread == g_current_thread) {
        return KERN_ABORTED;
    }

    // Remove from the parent task's thread list
    if (thread->parent_task) {
        struct thread **current = &thread->parent_task->thread_list;
        while (*current) {
            if (*current == thread) {
                *current = thread->next;
                if (thread->parent_task->thread_count != 0) {
                    thread->parent_task->thread_count--;
                }
                break;
            }
            current = &(*current)->next;
        }
    }

    if (thread->kstack_base) {
        kfree((void *)thread->kstack_base);
    }

    kfree(thread);
    return KERN_SUCCESS;
}

kern_return_t thread_blockon(thread_t *thread, ipc_port_t *port) {
    if (thread == NULL || port == NULL) {
        return KERN_INVALID_ARGUMENT;
    }
    return KERN_ABORTED;
}

kern_return_t thread_unblock(thread_t *thread, int32_t result) {
    if (thread == NULL) {
        return KERN_INVALID_ARGUMENT;
    }
    (void)result;
    return KERN_ABORTED;
}