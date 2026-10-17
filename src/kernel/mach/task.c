#include "task.h"

#include <mach/thread.h>

/* Kernel bootstrap task */
struct task kernel_task;
task_t *kernel_task_handle = &kernel_task;

kern_return_t task_initialize_kernel() {
	// STUB: Implement kernel task initialization
	STUB("task_initialize_kernel");
}

kern_return_t task_create(task_t *parent_task) {
	// STUB: Implement task create
	STUB("task_create");
}

kern_return_t task_destroy(task_t *task) {
	// STUB: Implement task destroy
	STUB("task_destroy");
}

kern_return_t task_suspend(task_t *task) {
	// STUB: Implement task suspend
	STUB("task_suspend");
}

kern_return_t task_resume(task_t *task) {
	// STUB: Implement task resume
	STUB("task_resume");
}