#ifndef CELESTEOS_PSTASK_H
#define CELESTEOS_PSTASK_H

#include <ke/page.h>
#include <ke/spinlock.h>
#include <ke/svc.h>
#include <ob/obheader.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct PsThread PsThread;

#define MAX_HANDLES_PER_TASK 256
#define MAX_TASKS 1024

/* Unique task identifier */
typedef int64_t PsTaskId;

enum {
    PS_ACCESS_READ = 1U << 0,
    PS_ACCESS_WRITE = 1U << 1,
    PS_ACCESS_CALL = 1U << 2,
    PS_ACCESS_GRANT = 1U << 3,
};

typedef struct PsHandle {
    ObHeader *object;

    /* Access rights for this handle */
    uint32_t access;

    /* Generation number for this handle, incremented on each reuse */
    uint64_t generation;
} PsHandle;

typedef struct PsTask {
    ObHeader object;
    KeSpinLock handleLock;

    /* Hold the process ID for this task */
    PsTaskId pid;

    /* Virtual address of the task's page table */
    KePageTable *pageTable;

    PsHandle handles[MAX_HANDLES_PER_TASK];
    /* Number of threads attached to this task */
    size_t threadCount;
} PsTask;

/**
 * Initialize the task subsystem.
 *
 * This function should be called once early in the kernel initialization process.
 */
void PsTaskSubsystemInit(uint64_t hhdmBase);

/**
 * Create a new task, returning a pointer to the newly created task.
 */
PsTask *PsTaskCreate(void);

/**
 * Return the currently executing task.
 */
PsTask *PsCurrentTask(void);

/**
 * Insert a handle into the specified task's handle table.
 *
 * Returns a service handle representing the inserted handle.
 */
KeSvcHandle PsInsertTaskHandle(PsTask *task, ObHeader *object, uint32_t access);

/**
 * Retrieve an object if the handle is valid and grants all required access.
 *
 * The returned object has a retained reference; the caller must release it.
 */
ObHeader *PsGetTaskHandleObject(PsTask *task, KeSvcHandle handle, uint32_t requiredAccess);

/**
 * Release a handle from the specified task's handle table.
 *
 * Returns true if the handle was successfully released, or false if it was invalid.
 */
bool PsTaskReleaseHandle(PsTask *task, KeSvcHandle handle);

/**
 * Attach a thread to the specified task.
 */
void PsTaskAttachThread(PsTask *task, PsThread *thread);

/**
 * Detach a thread from the specified task.
 */
void PsTaskDetachThread(PsTask *task, PsThread *thread);

#endif // CELESTEOS_PSTASK_H
