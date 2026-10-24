#ifndef IPC_H
#define IPC_H 1

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SYS_IPC_SEND 1
#define SYS_IPC_RECV 2
#define SYS_IPC_CALL 3
#define SYS_IPC_VM_MAP 4
#define SYS_IPC_THREAD 5

typedef enum {
    THREAD_INACTIVE = 0,
    THREAD_READY,
    THREAD_BLOCKED_SEND,
    THREAD_BLOCKED_RECV
} thread_state_t;

typedef uint8_t cap_handle_t;

typedef struct {
    cap_handle_t capability;
} handle_t;

#endif