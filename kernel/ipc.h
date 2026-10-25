#ifndef CELESTEOS_IPC_H
#define CELESTEOS_IPC_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <kernel/ipc/thread.h>

#define SYS_IPC_SEND 1
#define SYS_IPC_RECV 2
#define SYS_IPC_CALL 3
#define SYS_IPC_VM_MAP 4
#define SYS_IPC_THREAD 5

typedef uint8_t cap_handle_t;

typedef struct {
    cap_handle_t capability;
} handle_t;

#endif // CELESTEOS_IPC_H