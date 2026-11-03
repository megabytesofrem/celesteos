#include <ps/pstask.h>

#include <error.h>
#include <ob/obcreation.h>
#include <ps/psthread.h>

#include <limits.h>
#include <stdatomic.h>

static PsTask gTasks[MAX_TASKS];
static ObObjectPool gTaskPool;
static uint64_t gHhdmBase;
static _Atomic uint64_t gNextPid = 1;

#define PS_HANDLE_INDEX_BITS 8U
#define PS_HANDLE_INDEX_MASK ((UINT64_C(1) << PS_HANDLE_INDEX_BITS) - 1)
#define PS_HANDLE_GENERATION_MASK ((uint64_t)INT64_MAX >> PS_HANDLE_INDEX_BITS)

static void PsTaskCreateObject(ObHeader *object, ObObjectData data);
static void PsTaskDestroyObject(ObHeader *object);

static const ObOperations gTaskOperations = {
    .name = "task",
    .create = PsTaskCreateObject,
    .destroy = PsTaskDestroyObject,
};

static KeSvcHandle PsEncodeHandle(size_t index, const PsHandle *entry) {
    uint64_t encoded = (entry->generation << PS_HANDLE_INDEX_BITS) | index;
    return (KeSvcHandle)encoded;
}

static bool PsDecodeHandle(KeSvcHandle handle, size_t *index, uint64_t *generation) {
    if (handle < 0 || !index || !generation) {
        return false;
    }

    uint64_t encoded = (uint64_t)handle;
    *index = (size_t)(encoded & PS_HANDLE_INDEX_MASK);
    *generation = encoded >> PS_HANDLE_INDEX_BITS;
    return *index < MAX_HANDLES_PER_TASK;
}

void PsTaskSubsystemInit(uint64_t hhdmBase) {
    gHhdmBase = hhdmBase;
    ObObjectPoolInit(&gTaskPool, gTasks, sizeof(gTasks[0]), MAX_TASKS, &gTaskOperations);
    atomic_store_explicit(&gNextPid, 1, memory_order_relaxed);
}

static void PsTaskCreateObject(ObHeader *object, ObObjectData data) {
    (void)data;
    PsTask *task = (PsTask *)object;
    KeSpinLockInit(&task->handleLock);
    task->pid = (PsTaskId)atomic_fetch_add_explicit(&gNextPid, 1, memory_order_relaxed);
    task->pageTable = KeCreateUserPageTable(gHhdmBase);
    task->threadCount = 0;
}

static void PsTaskDestroyObject(ObHeader *object) {
    PsTask *task = (PsTask *)object;

    for (size_t i = 0; i < MAX_HANDLES_PER_TASK; ++i) {
        PsHandle *entry = &task->handles[i];
        if (entry->object) {
            ObHeader *heldObject = entry->object;
            entry->object = NULL;
            entry->access = 0;
            entry->generation = (entry->generation + 1) & PS_HANDLE_GENERATION_MASK;
            ObRelease(heldObject);
        }
    }

    task->pid = 0;
    task->pageTable = NULL;
    task->threadCount = 0;
}

PsTask *PsTaskCreate(void) {
    PsTask *task = (PsTask *)ObCreateInPool(&gTaskPool, ObObjectTypeTask, NULL);
    if (!task) {
        return NULL;
    }

    if (!task->pageTable) {
        ObRelease(&task->object);
        return NULL;
    }

    return task;
}

PsTask *PsCurrentTask(void) {
    PsThread *thread = PsCurrentThread();
    return thread ? thread->parentTask : NULL;
}

KeSvcHandle PsInsertTaskHandle(PsTask *task, ObHeader *object, uint32_t access) {
    if (!task || !object) {
        return INVALID_HANDLE;
    }

    KeSpinLockAcquire(&task->handleLock);
    for (size_t i = 0; i < MAX_HANDLES_PER_TASK; ++i) {
        PsHandle *entry = &task->handles[i];
        if (!entry->object) {
            ObRetain(object);
            entry->object = object;
            entry->access = access;
            KeSvcHandle handle = PsEncodeHandle(i, entry);
            KeSpinLockRelease(&task->handleLock);
            return handle;
        }
    }
    KeSpinLockRelease(&task->handleLock);
    return INVALID_HANDLE;
}

ObHeader *PsGetTaskHandleObject(PsTask *task, KeSvcHandle handle, uint32_t requiredAccess) {
    size_t index;
    uint64_t generation;
    if (!task || !PsDecodeHandle(handle, &index, &generation)) {
        return NULL;
    }

    KeSpinLockAcquire(&task->handleLock);
    PsHandle *entry = &task->handles[index];
    ObHeader *object = NULL;

    // Validate the handle by checking the object, generation, and required access rights
    if (entry->object && entry->generation == generation &&
        (entry->access & requiredAccess) == requiredAccess) {
        object = entry->object;
        ObRetain(object);
    }
    KeSpinLockRelease(&task->handleLock);
    return object;
}

bool PsTaskReleaseHandle(PsTask *task, KeSvcHandle handle) {
    size_t index;
    uint64_t generation;
    if (!task || !PsDecodeHandle(handle, &index, &generation)) {
        return false;
    }

    KeSpinLockAcquire(&task->handleLock);
    PsHandle *entry = &task->handles[index];
    if (!entry->object || entry->generation != generation) {
        KeSpinLockRelease(&task->handleLock);
        return false;
    }

    ObHeader *object = entry->object;
    entry->object = NULL;
    entry->access = 0;
    entry->generation = (entry->generation + 1) & PS_HANDLE_GENERATION_MASK;
    KeSpinLockRelease(&task->handleLock);
    ObRelease(object);
    return true;
}

void PsTaskAttachThread(PsTask *task, PsThread *thread) {
    if (task && thread) {
        __atomic_add_fetch(&task->threadCount, 1, __ATOMIC_RELAXED);
    }
}

void PsTaskDetachThread(PsTask *task, PsThread *thread) {
    if (!task || !thread) {
        return;
    }

    size_t threadCount = __atomic_load_n(&task->threadCount, __ATOMIC_RELAXED);
    while (threadCount != 0 && !__atomic_compare_exchange_n(&task->threadCount, &threadCount,
                                   threadCount - 1, true, __ATOMIC_RELAXED, __ATOMIC_RELAXED)) {
    }
}
