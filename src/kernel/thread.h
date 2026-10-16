#ifndef CELESTEOS_THREAD_H
#define CELESTEOS_THREAD_H

#include <stdint.h>

/* Forward declarations for kernel thread-related structures */
struct mach_kmsg;
struct mach_port_space;

typedef enum {
    THREAD_RUNNING,
    THREAD_WAITING, /* waiting for IPC */
    THREAD_DEAD     /* terminated */
} thread_state_t;

typedef struct cpu_context {
    uint64_t r15, r14, r13, r12; /* callee-saved registers */
    uint64_t rbp, rbx;           /* base pointer and base register */
    uint64_t rip;                /* instruction pointer */
    uint64_t rsp;                /* stack pointer */
    uint64_t rflags;             /* flags register */
} cpu_context_t;

typedef struct thread {
    uint64_t kstack;       /* kernel stack pointer */
    cpu_context_t context; /* saved CPU context */
    thread_state_t state;  /* current scheduler state */

    struct mach_port_space* port_space; /* tasks port space */
    uint32_t waiting_on_port;           /* the port we are waiting on */
    struct mach_kmsg* received_msg;     /* pointer to the received message */
} thread_t;

#endif  // CELESTEOS_THREAD_H