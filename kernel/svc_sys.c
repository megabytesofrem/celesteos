#include "svc_sys.h"

#include <klog.h>

int sys_svc_send_msg(handle_t target, svc_message_t *message, svc_message_t *recv_buffer) {
    // Implementation goes here
    klog(KLOG_INFO, "svc: Sending message to target: %d", target);
}

int sys_svc_reply_wait(handle_t client, svc_message_t *message, svc_message_t *recv_buffer) {
    // Implementation goes here
    klog(KLOG_INFO, "svc: Replying and waiting for client: %d", client);
}

int sys_svc_vm_map(handle_t target, void *virt_addr, void *phys_addr, int flags) {
    // Implementation goes here
    klog(KLOG_INFO, "svc: Mapping memory for target: %d", target);
}

int sys_svc_vm_unmap(handle_t target, void *virt_addr) {
    // Implementation goes here
    klog(KLOG_INFO, "svc: Unmapping memory for target: %d", target);
}

int sys_svc_thread_create(handle_t target, void *entry_point, void *user_stack_top) {
    // Implementation goes here
    klog(KLOG_INFO, "svc: Creating thread for target: %d", target);
}

int sys_svc_thread_yield(handle_t target) {
    // Implementation goes here
    klog(KLOG_INFO, "svc: Yielding thread for target: %d", target);
}

int sys_dispatch(int64_t svc_number, svc_cpu_frame_t *frame) {
    klog(KLOG_INFO, "svc: Dispatching service call: %d", svc_number);

    switch (svc_number) {
    case SVC_SEND_MSG:
        return sys_svc_send_msg((handle_t)frame->rdi, (svc_message_t *)frame->rsi,
                                (svc_message_t *)frame->rdx);
    case SVC_REPLY_WAIT:
        return sys_svc_reply_wait((handle_t)frame->rdi, (svc_message_t *)frame->rsi,
                                  (svc_message_t *)frame->rdx);
    case SVC_VM_MAP:
        return sys_svc_vm_map((handle_t)frame->rdi, (void *)frame->rsi, (void *)frame->rdx,
                              (int)frame->r8);
    case SVC_VM_UNMAP:
        return sys_svc_vm_unmap((handle_t)frame->rdi, (void *)frame->rsi);
    case SVC_THREAD_CREATE:
        return sys_svc_thread_create((handle_t)frame->rdi, (void *)frame->rsi, (void *)frame->rdx);
    case SVC_THREAD_YIELD:
        return sys_svc_thread_yield((handle_t)frame->rdi);
    default:
        klog(KLOG_ERROR, "svc: Unknown service call: %ld", svc_number);
        return -1; // Unknown service call
    }
}