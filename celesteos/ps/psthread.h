#ifndef CELESTEOS_PSTHREAD_H
#define CELESTEOS_PSTHREAD_H

#include <ke/svc.h>
#include <ob/obheader.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct PsTask PsTask;
/* Unique thread identifier */
typedef int64_t PsThreadId;

typedef enum PsThreadState {
    PsThreadStateInactive,
    PsThreadStateRunning,
    PsThreadStateReady,
    PsThreadStateBlockedSend,
    PsThreadStateBlockedReceive,
} PsThreadState;

typedef struct PsThread {
    /* Common header for all objects */
    ObHeader object;
    PsThreadId tid;
    PsThreadState state;
    uint64_t *rsp;
    uint64_t rip;
    uint64_t rflags;
    uintptr_t userStack;
    /* Kernel stack pointer for this thread object */
    uint8_t kernelStack[8192] __attribute__((aligned(16)));

    /* Parent task of this thread object */
    PsTask *parentTask;

    /* IPC partner and buffers */
    PsThreadId ipcPartnerId;
    void *ipcReceiveBuffer; /* Buffer for receiving IPC messages */
    SvcMessage ipcMessage;  /* Fast path IPC registers */
    bool schedulerReferenceHeld;

    /* Pointer to the previous and next thread objects */
    struct PsThread *prev;
    struct PsThread *next;
} PsThread;

/**
 * Initialize the thread subsystem once during kernel initialization.
 */
void PsThreadSubsystemInit(void);

/**
 * Return the next unique thread identifier.
 */
PsThreadId PsNextThreadId(void);

/**
 * Return the currently executing thread.
 */
PsThread *PsCurrentThread(void);

/**
 * Create a thread within the specified parent task.
 *
 * The entryPoint parameter specifies the starting address of the thread's execution.
 * The userStack parameter specifies the initial user stack pointer for the thread.
 *
 * The new thread is not runnable until it is passed to PsSchedulerEnqueueThread.
 */
PsThread *PsCreateThread(PsTask *parentTask, void *entryPoint, void *userStack);

#endif // CELESTEOS_PSTHREAD_H
