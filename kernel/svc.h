#ifndef CELESTEOS_SVC_H
#define CELESTEOS_SVC_H

#include <stddef.h>
#include <stdint.h>

typedef int64_t thread_id_t;
typedef int64_t task_id_t;

typedef enum {
    SVC_SEND_MSG,
    SVC_REPLY_WAIT,
    SVC_VM_MAP,
    SVC_VM_UNMAP,
    SVC_THREAD_CREATE,
    SVC_THREAD_YIELD
} svc_number_t;

typedef enum {
    SVC_CAP_MEMORY_MAP,   /* Map memory into the address space of the reciever */
    SVC_CAP_MEMORY_GRANT, /* Transfer memory ownership to another task */
    SVC_CAP_IPC_ENDPOINT, /* Capability for sending IPC messages to another task */
    SVC_CAP_THREAD,       /* Capability for managing threads */
    SVC_CAP_NOTIFY,       /* Capability for sending notifications */
} svc_capability_type_t;

typedef struct svc_message_tag {
    uint16_t label;
    uint8_t register_count;   /* Number of registers being sent*/
    uint8_t capability_count; /* Number of capabilities being sent*/
    uint32_t flags;
} svc_message_tag_t;

typedef struct svc_capability {
    uint32_t type;       /* Capability type */
    uintptr_t base_addr; /* Base address of the capability */
    size_t size_pages;   /* Size of the capability in pages */
    uint32_t rights;     /* Rights associated with the capability */
} svc_capability_t;

typedef struct svc_message {
    svc_message_tag_t tag;
    uintptr_t registers[4];           /* Array of registers being sent */
    svc_capability_t capabilities[8]; /* Array of capabilities being sent */
} svc_message_t;

int svc_send_msg(thread_id_t target_tid, svc_message_t *message, svc_message_t *recv_buffer);
int svc_reply_wait(thread_id_t client_tid, svc_message_t *message, svc_message_t *recv_buffer);
int svc_vm_map(task_id_t target_task, void *virt_addr, void *phys_addr, int flags);
int svc_vm_unmap(task_id_t target_task, void *virt_addr);
int svc_thread_create(task_id_t target_task, void *entry_point, void *user_stack_top);
int svc_thread_yield(thread_id_t target_tid);

#endif // CELESTEOS_SVC_H