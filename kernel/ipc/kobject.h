#ifndef CELESTEOS_KOBJECT_H
#define CELESTEOS_KOBJECT_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    KOBJ_TYPE_NONE,
    KOBJ_TYPE_THREAD,   /* A thread object */
    KOBJ_TYPE_TASK,     /* A task object */
    KOBJ_TYPE_ENDPOINT, /* An IPC endpoint object */
    KOBJ_TYPE_PAGE,     /* A memory page object */
    KOBJ_TYPE_IRQ,      /* An interrupt request object */
} kobject_type_t;

typedef struct kobject {
    kobject_type_t type;
    _Atomic uint32_t ref_count;
} kobject_t;

static kobject_t kobject_create(kobject_type_t type) {
    kobject_t kobj = { .type = type, .ref_count = 1 };
    return kobj;
}

static inline void kobject_init(kobject_t *kobj, kobject_type_t type) {
    if (!kobj) {
        return;
    }

    kobj->type = type;
    kobj->ref_count = 1;
}

static inline void kobject_retain(kobject_t *kobj) {
    if (!kobj) {
        return;
    }

    __atomic_fetch_add(&kobj->ref_count, 1, __ATOMIC_RELAXED);
}

static inline bool kobject_release(kobject_t *kobj) {
    if (!kobj) {
        return false;
    }

    if (__atomic_fetch_sub(&kobj->ref_count, 1, __ATOMIC_RELEASE) == 1) {
        // Last reference released, caller needs to free the object
        __atomic_thread_fence(__ATOMIC_ACQUIRE);
        return true;
    }

    return false;
}

#endif // CELESTEOS_KOBJECT_H