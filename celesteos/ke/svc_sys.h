#ifndef CELESTEOS_SVC_SYS_H
#define CELESTEOS_SVC_SYS_H

#include <stddef.h>
#include <stdint.h>

#include "svc.h"

typedef struct __attribute__((packed)) KeSvcCPUFrame {
    /* Saved Registers */
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;

    /* Hardware Return Frame */
    uint64_t rip;
    uint64_t cs; /* Ring 3 Code Segment */
    uint64_t rflags;
    uint64_t rsp; /* Userland RSP */
    uint64_t ss;  /* Ring 3 Data Segment */
} KeSvcCPUFrame;

/* Service call system interface */

int KeSvcSendMessage(KeSvcHandle target, SvcMessage *message, SvcMessage *recv_buffer);
int KeSvcReplyWait(KeSvcHandle client, SvcMessage *reply, SvcMessage *recv_buffer);
int KeSvcVMMap(KeSvcHandle target, void *virt_addr, void *phys_addr, int flags);
int KeSvcVMUnmap(KeSvcHandle target, void *virt_addr);
int KeSvcCreateThread(KeSvcHandle target, void *entry_point, void *user_stack_top);
int KeSvcYieldThread(KeSvcHandle target);
int KeSvcUnknownService(KeSvcHandle target);

extern int KeSvcDispatch(int64_t svcNumber, KeSvcCPUFrame *frame);
extern void KeCallSystemService(void);
extern uint64_t KeSyscallKernelStack;

#endif // CELESTEOS_SVC_SYS_H