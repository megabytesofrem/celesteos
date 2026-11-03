#ifndef CELESTEOS_SVC_H
#define CELESTEOS_SVC_H

#include <stddef.h>
#include <stdint.h>

typedef int64_t KeSvcHandle;

#define INVALID_HANDLE ((KeSvcHandle) - 1)

/**
 * Service call numbers
 */
typedef enum {
    SVC_SEND_MSG,
    SVC_REPLY_WAIT,
    SVC_VM_MAP,
    SVC_VM_UNMAP,
    SVC_THREAD_CREATE,
    SVC_THREAD_YIELD
} KeSvcNumber;

/**
 * Capability descriptor for a service call.
 */
typedef enum {
    SVC_CAP_MEMORY_MAP,   /* Map memory into the address space of the reciever */
    SVC_CAP_MEMORY_GRANT, /* Transfer memory ownership to another task */
    SVC_CAP_IPC_ENDPOINT, /* Capability for sending IPC messages to another task */
    SVC_CAP_THREAD,       /* Capability for managing threads */
    SVC_CAP_NOTIFY,       /* Capability for sending notifications */
} KeSvcCapabilityType;

typedef struct {
    uint16_t label;
    uint8_t registerCount;   /* Number of registers being sent*/
    uint8_t capabilityCount; /* Number of capabilities being sent*/
    uint32_t flags;
} SvcMessageTag;

typedef struct {
    uint32_t type;       /* Capability type */
    uintptr_t baseAddr; /* Base address of the capability */
    size_t sizePages;   /* Size of the capability in pages */
    uint32_t rights;     /* Rights associated with the capability */
} SvcCapability;

typedef struct {
    SvcMessageTag tag;
    uintptr_t registers[4];        /* Array of registers being sent */
    SvcCapability capabilities[8]; /* Capability descriptors */
} SvcMessage;

#endif // CELESTEOS_SVC_H