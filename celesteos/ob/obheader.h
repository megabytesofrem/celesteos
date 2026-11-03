#ifndef CELESTEOS_OBHEADER_H
#define CELESTEOS_OBHEADER_H

#include <stdbool.h>
#include <stdint.h>

typedef struct ObHeader ObHeader;
typedef struct ObObjectPool ObObjectPool;

typedef void *ObObjectData;
typedef void (*ObCreateFunc)(ObHeader *object, ObObjectData data);
typedef void (*ObDestroyFunc)(ObHeader *object);

typedef enum ObObjectType {
    ObObjectTypeNone,
    ObObjectTypeThread,   /* A thread object */
    ObObjectTypeTask,     /* A task object */
    ObObjectTypeEndpoint, /* An IPC endpoint object */
    ObObjectTypePage,     /* A memory page object */
    ObObjectTypeIrq,      /* An interrupt request object */
} ObObjectType;

struct ObHeader {
    ObObjectType type;
    _Atomic uint32_t references; /* Reference count for the object */
    ObObjectPool *pool;
    ObHeader *nextFree; /* Next object in the free list */
};

typedef struct ObOperations {
    const char *name;
    ObCreateFunc create;
    ObDestroyFunc destroy;
} ObOperations;

#endif // CELESTEOS_OBHEADER_H
