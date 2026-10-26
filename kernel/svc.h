#ifndef CELESTEOS_SVC_H
#define CELESTEOS_SVC_H

typedef enum {
    SVC_SEND_MSG,
    SVC_REPLY_WAIT,
    SVC_VM_MAP,
    SVC_VM_UNMAP,
    SVC_THREAD_CREATE,
    SVC_THREAD_YIELD
} svc_number_t;

typedef void *svc_message_t;

int svc_send_msg(int target_tid, svc_message_t *message, void *recv_buffer);
int svc_reply_wait(int client_tid, svc_message_t *message, void *recv_buffer);
int svc_vm_map(int target_task, void *virt_addr, void *phys_addr, int flags);
int svc_vm_unmap(int target_task, void *virt_addr);
int svc_thread_create(int target_task, void *entry_point, void *user_stack_top);
int svc_thread_yield(int target_tid);

#endif // CELESTEOS_SVC_H