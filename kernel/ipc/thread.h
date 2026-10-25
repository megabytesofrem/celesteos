#ifndef CELESTEOS_THREAD_H
#define CELESTEOS_THREAD_H

#include <stddef.h>
#include <stdint.h>

typedef enum {
    THREAD_INACTIVE,
    THREAD_RUNNING,
    THREAD_READY,
    THREAD_BLOCKED_SEND,
    THREAD_BLOCKED_RECV
} thread_state_t;

typedef struct thread {
    uint64_t tid;
    thread_state_t state;

    uint64_t rsp;
    uint64_t rip;
    uint64_t rflags;

    /* Kernel stack for this thread */
    uint8_t kernel_stack[8192] __attribute__((aligned(16)));

    /* Parent task of this thread */
    struct task *parent_task;

    uint64_t ipc_partner_tid;
    uint64_t ipc_msg_payload[6];

    struct thread *next;
    struct thread *prev;
} thread_t;

#endif // CELESTEOS_THREAD_H