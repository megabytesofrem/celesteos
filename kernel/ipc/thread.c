#include "thread.h"
#include "scheduler.h"
#include "task.h"

#include <kernel/tss.h>

#include <string.h>

thread_t g_thread_pool[MAX_THREADS];
_Atomic thread_id_t current_thread_id = 0;

static void thread_start(void) __attribute__((noreturn));

static void thread_start(void) {
    thread_t *thread = scheduler_get_current();
    if (!thread) {
        __builtin_trap();
    }

    tss_switch_ring3(thread->rip, thread->user_rsp);
    __builtin_unreachable();
}

thread_id_t next_thread_id(void) {
    // FIXME: Add spinlock to protect thread access in multi-core environments
    return __atomic_add_fetch(&current_thread_id, 1, __ATOMIC_SEQ_CST);
}

void thread_sys_init(void) {
    memset(g_thread_pool, 0, sizeof(g_thread_pool));

    for (int i = 0; i < MAX_THREADS; i++) {
        g_thread_pool[i].tid = -1;
        g_thread_pool[i].state = THREAD_INACTIVE;
    }
}

thread_t *thread_alloc_slot(void) {
    for (int i = 0; i < MAX_THREADS; i++) {
        // Look for an inactive thread slot in the pool
        if (g_thread_pool[i].state == THREAD_INACTIVE) {
            g_thread_pool[i].tid = next_thread_id();
            g_thread_pool[i].state = THREAD_READY;
            return &g_thread_pool[i];
        }
    }
    return NULL;
}

thread_t *thread_create(struct task *parent_task, void *entry_point, void *user_stack_top) {
    if (!parent_task || !parent_task->cr3_page_table || !entry_point || !user_stack_top) {
        return NULL;
    }

    // FIXME: Add spinlock to protect thread access in multi-core environments

    thread_t *thread = thread_alloc_slot();
    if (!thread) {
        return NULL;
    }

    thread->parent_task = parent_task;
    thread->rip = (uintptr_t)entry_point;
    thread->user_rsp = (uintptr_t)user_stack_top;
    thread->rflags = 0x202; /* default RFLAGS value, IF = 1 */
    thread->next = NULL;
    thread->prev = NULL;

    // Set up the initial kernel stack frame for the new thread
    uintptr_t *top = (uintptr_t *)(thread->kernel_stack + sizeof(thread->kernel_stack));

    // Align the stack pointer to a 16-byte boundary as required by the ABI
    top = (uintptr_t *)((uintptr_t)top & ~(uintptr_t)0xF);

    uintptr_t *frame = top - 7;
    frame[0] = 0;                       /* r15 */
    frame[1] = 0;                       /* r14 */
    frame[2] = 0;                       /* r13 */
    frame[3] = 0;                       /* r12 */
    frame[4] = 0;                       /* rbx */
    frame[5] = 0;                       /* rbp */
    frame[6] = (uintptr_t)thread_start; /* return address */

    thread->rsp = (uint64_t *)frame;

    return thread;
}
