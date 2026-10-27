#ifndef CELESTEOS_SVC_SYS_H
#define CELESTEOS_SVC_SYS_H

#include <stddef.h>
#include <stdint.h>

#include "svc.h"

typedef struct __attribute__((packed)) svc_cpu_frame {
    /* Saved Registers */
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;

    /* Hardware Return Frame */
    uint64_t rip;
    uint64_t cs; /* Ring 3 Code Segment */
    uint64_t rflags;
    uint64_t rsp; /* Userland RSP */
    uint64_t ss;  /* Ring 3 Data Segment */
} svc_cpu_frame_t;

/* Service call system interface */

int sys_svc_send_msg(handle_t target, svc_message_t *message, svc_message_t *recv_buffer);
int sys_svc_reply_wait(handle_t client, svc_message_t *message, svc_message_t *recv_buffer);
int sys_svc_vm_map(handle_t target, void *virt_addr, void *phys_addr, int flags);
int sys_svc_vm_unmap(handle_t target, void *virt_addr);
int sys_svc_thread_create(handle_t target, void *entry_point, void *user_stack_top);
int sys_svc_thread_yield(handle_t target);
int sys_svc_unknown(handle_t target);

extern int sys_dispatch(int64_t svc_number, svc_cpu_frame_t *frame);

#endif // CELESTEOS_SVC_SYS_H