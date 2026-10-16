#include "thread.h"

#include <kernel/memory/kmalloc.h>
#include <string.h>

#include "task.h"

static uintptr_t next_tid = 1;

static uintptr_t generate_tid() {
    return next_tid++;
}

thread_t* thread_create(task_t* parent, uintptr_t entry_point, uintptr_t arg) {
    thread_t* thread = kmalloc(sizeof(thread_t));

    // Initialize the thread structure
    spinlock_init(&thread->lock);
    thread->tid = generate_tid();

    // Allocate the kernel stack for the thread
    thread->kstack_base = (uintptr_t)kmalloc(KERNEL_STACK_SIZE);
    if (!thread->kstack_base) {
        kfree(thread);
        return NULL;
    }
    thread->kstack_top = thread->kstack_base + KERNEL_STACK_SIZE;

    // Setup the initial CPU context for the thread
    uintptr_t stack_ptr = thread->kstack_top & ~0xFUL; /* align stack pointer to 16 bytes */

    // Reserve space for the initial CPU context on the stack
    stack_ptr -= sizeof(cpu_context_t);
    cpu_context_t* cpu_ctx = (cpu_context_t*)stack_ptr;
    memset(cpu_ctx, 0, sizeof(cpu_context_t));

    // Populate initial CPU context
    cpu_ctx->rip = entry_point;
    cpu_ctx->rsp = stack_ptr;
    cpu_ctx->rflags = 0x202; /* default flags with interrupts enabled */
    cpu_ctx->rbx = arg;      /* pass the argument to the thread */

    // NOTE: Callee saved registers (R12-R15, RBP) are already initialized to 0

    // Save the initial CPU context pointer in the thread structure
    thread->context = cpu_ctx;

    // Initialize the parent task and thread state
    thread->parent_task = parent;
    thread->state = THREAD_STOPPED;

    return thread;
}

void thread_destroy(thread_t* thread) {
    if (!thread) return;

    // Remove from the parent task's thread list
    if (thread->parent_task) {
        // Traverse the thread list of the parent task to remove the current thread
        struct thread** current = &thread->parent_task->thread_list;
        while (*current) {
            // Check if the current thread in the list is the one to be removed
            if (*current == thread) {
                // Unlink the thread from the list
                *current = thread->next;
                break;
            }

            // Update current, move to the next thread in the list
            current = &(*current)->next;
        }
    }

    // Free the kernel stack
    if (thread->kstack_base) {
        kfree((void*)thread->kstack_base);
    }

    // Free the thread structure
    kfree(thread);
}

void thread_blockon(thread_t* thread, ipc_port_t* port) {
    if (!thread) return;

    // Lock the thread before modifying its state
    spinlock_acquire(&thread->lock);
    thread->waiting_on_port = port;
    thread->state = THREAD_WAITING;
    // Unlock the thread after modifying its state
    spinlock_release(&thread->lock);
}

void thread_unblock(thread_t* thread) {
    if (!thread) return;

    // Lock the thread before modifying its state
    spinlock_acquire(&thread->lock);
    thread->waiting_on_port = NULL;
    thread->state = THREAD_RUNNING;
    // Unlock the thread after modifying its state
    spinlock_release(&thread->lock);
}