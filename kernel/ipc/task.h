#ifndef CELESTEOS_TASK_H
#define CELESTEOS_TASK_H

#include <stddef.h>
#include <stdint.h>

#include "kobject.h"

#define MAX_HANDLES_PER_TASK 256

#define CAP_RIGHT_READ (1 << 0)
#define CAP_RIGHT_WRITE (1 << 1)
#define CAP_RIGHT_CALL (1 << 2)
#define CAP_RIGHT_GRANT (1 << 3)

typedef struct handle_entry {
    kobject_t *kobj;
    uint32_t rights;
    void *target;
} handle_entry_t;

typedef struct task {
    kobject_t kobj;

    uint64_t pid;
    /* HHDM virtual address of the task's PML4 */
    struct page_table *cr3_page_table;

    struct handle_entry handles[MAX_HANDLES_PER_TASK];
    uint64_t thread_count;
} task_t;

#endif // CELESTEOS_TASK_H