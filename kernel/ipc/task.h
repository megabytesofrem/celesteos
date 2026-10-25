#ifndef CELESTEOS_TASK_H
#define CELESTEOS_TASK_H

#include <stddef.h>
#include <stdint.h>

#define MAX_HANDLES_PER_TASK 256

typedef enum {
    CAP_TYPE_NONE,
    CAP_TYPE_THREAD,
    CAP_TYPE_TASK,
    CAP_TYPE_IRQ,
} capability_type_t;

#define CAP_RIGHT_READ (1 << 0)
#define CAP_RIGHT_WRITE (1 << 1)
#define CAP_RIGHT_CALL (1 << 2)
#define CAP_RIGHT_GRANT (1 << 3)

typedef struct capability_entry {
    capability_type_t type;
    uint32_t rights;
    void *target;
} capability_entry_t;

typedef struct task {
    uint64_t pid;
    uint64_t cr3_page_table;

    struct capability_entry *handles[MAX_HANDLES_PER_TASK];
    uint64_t thread_count;
} task_t;

#endif // CELESTEOS_TASK_H