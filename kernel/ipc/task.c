#include "task.h"

#include <ipc/ipc_port.h>
#include <ipc/thread.h>
#include <kernel/kmalloc.h>
#include <klog.h>
#include <string.h>

task_t kernel_task;
task_t *kernel_task_handle = &kernel_task;

static uint32_t next_task_id = 2;

static kern_return_t task_create_self_port(task_t *task, mach_port_name_t *name) {
    ipc_port_t *port;
    kern_return_t result = ipc_port_alloc(task->itk_space, 0, name, &port);
    if (result == KERN_SUCCESS) {
        ipc_port_release(port);
    }
    return result;
}

kern_return_t task_initialize_kernel(void) {
    if (kernel_task.itk_space != NULL) {
        return KERN_ALREADY_EXISTS;
    }
    memset(&kernel_task, 0, sizeof(kernel_task));

    // Initialize the kernel task structure.
    spinlock_init(&kernel_task.lock);
    kernel_task.task_id = 1;
    kernel_task.state = TASK_ACTIVE;

    // Create the IPC space for the kernel task.
    kern_return_t result = ipc_space_create(&kernel_task.itk_space);
    if (result != KERN_SUCCESS) {
        return result;
    }

    // Create the Mach thread for the kernel task
    thread_t *initial_thread;
    result = thread_create(&kernel_task, 0, 0, &initial_thread);
    if (result != KERN_SUCCESS) {
        (void)ipc_space_destroy(kernel_task.itk_space);
        kernel_task.itk_space = NULL;
        return result;
    }
    initial_thread->state = THREAD_RUNNING;

    // Create the self IPC port for the kernel task and its initial thread.
    result = task_create_self_port(&kernel_task, &kernel_task.itk_self);
    if (result == KERN_SUCCESS) {
        result = task_create_self_port(&kernel_task, &initial_thread->itk_self);
    }
    if (result != KERN_SUCCESS) {
        (void)thread_destroy(initial_thread);
        (void)ipc_space_destroy(kernel_task.itk_space);
        memset(&kernel_task, 0, sizeof(kernel_task));
        return result;
    }

    g_current_thread = initial_thread;
    klog(KLOG_INFO, "ipc: initialized bootstrap task %u and thread %u\n", kernel_task.task_id,
         initial_thread->tid);
    return KERN_SUCCESS;
}

kern_return_t task_create(task_t *parent_task, task_t **out_task) {
    if (out_task == NULL) {
        return KERN_INVALID_ARGUMENT;
    }
    *out_task = NULL;

    task_t *task = kmalloc(sizeof(*task));
    if (task == NULL) {
        return KERN_NO_MEMORY;
    }
    memset(task, 0, sizeof(*task));
    spinlock_init(&task->lock);
    task->task_id = __atomic_fetch_add(&next_task_id, 1, __ATOMIC_RELAXED);
    task->state = TASK_ACTIVE;
    if (parent_task != NULL) {
        task->uid = parent_task->uid;
        task->gid = parent_task->gid;
    }

    kern_return_t result = ipc_space_create(&task->itk_space);
    if (result != KERN_SUCCESS) {
        kfree(task);
        return result;
    }
    *out_task = task;
    return KERN_SUCCESS;
}

kern_return_t task_destroy(task_t *task) {
    if (task == NULL || task == &kernel_task || task == current_task()) {
        return KERN_INVALID_ARGUMENT;
    }

    while (task->thread_list != NULL) {
        if (task->thread_list == current_thread()) {
            return KERN_ABORTED;
        }
        (void)thread_destroy(task->thread_list);
    }
    if (task->itk_space != NULL) {
        kern_return_t result = ipc_space_destroy(task->itk_space);
        if (result != KERN_SUCCESS) {
            return result;
        }
    }
    task->state = TASK_TERMINATED;
    kfree(task);
    return KERN_SUCCESS;
}

kern_return_t task_suspend(task_t *task) {
    if (task == NULL) {
        return KERN_INVALID_ARGUMENT;
    }
    klog(KLOG_WARN, "task: suspend requires a context-switch scheduler\n");
    return KERN_ABORTED;
}

kern_return_t task_resume(task_t *task) {
    if (task == NULL) {
        return KERN_INVALID_ARGUMENT;
    }
    klog(KLOG_WARN, "task: resume requires a context-switch scheduler\n");
    return KERN_ABORTED;
}
