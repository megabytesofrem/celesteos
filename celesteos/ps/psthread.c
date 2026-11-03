#include <ps/psthread.h>

#include <ke/tss.h>
#include <ob/obcreation.h>
#include <ps/pstask.h>
#include <ps/sched.h>

#include <stdatomic.h>

#define MAX_THREADS 1024

typedef struct PsThreadCreateData {
    PsTask *parentTask;
    uintptr_t entryPoint;
    uintptr_t userStack;
} PsThreadCreateData;

static PsThread gThreads[MAX_THREADS];
static ObObjectPool gThreadPool;
static _Atomic PsThreadId gNextThreadId;

static void PsThreadCreateObject(ObHeader *object, ObObjectData data);
static void PsThreadDestroyObject(ObHeader *object);
static void PsThreadStart(void) __attribute__((noreturn));

static const ObOperations gThreadOperations = {
    .name = "thread",
    .create = PsThreadCreateObject,
    .destroy = PsThreadDestroyObject,
};

void PsThreadSubsystemInit(void) {
    ObObjectPoolInit(&gThreadPool, gThreads, sizeof(gThreads[0]), MAX_THREADS, &gThreadOperations);
    atomic_store_explicit(&gNextThreadId, 0, memory_order_relaxed);
}

PsThreadId PsNextThreadId(void) {
    return atomic_fetch_add_explicit(&gNextThreadId, 1, memory_order_relaxed) + 1;
}

PsThread *PsCurrentThread(void) {
    return PsSchedulerCurrentThread();
}

static void PsThreadCreateObject(ObHeader *object, ObObjectData data) {
    PsThread *thread = (PsThread *)object;
    const PsThreadCreateData *createData = data;

    thread->tid = PsNextThreadId();
    thread->state = PsThreadStateInactive;
    thread->rip = createData->entryPoint;
    thread->userStack = createData->userStack;
    thread->rflags = UINT64_C(0x202);
    thread->ipcPartnerId = -1;
    thread->parentTask = createData->parentTask;
    ObRetain(&thread->parentTask->object);
    PsTaskAttachThread(thread->parentTask, thread);

    uintptr_t stackTop = (uintptr_t)(thread->kernelStack + sizeof(thread->kernelStack));
    stackTop &= ~(uintptr_t)0xF;
    stackTop -= sizeof(uintptr_t);
    uintptr_t *frame = (uintptr_t *)(stackTop - 7 * sizeof(uintptr_t));
    for (size_t i = 0; i < 6; ++i) {
        frame[i] = 0;
    }
    frame[6] = (uintptr_t)PsThreadStart;
    thread->rsp = (uint64_t *)frame;
}

static void PsThreadDestroyObject(ObHeader *object) {
    PsThread *thread = (PsThread *)object;
    PsTask *parentTask = thread->parentTask;
    thread->tid = -1;
    thread->state = PsThreadStateInactive;
    thread->parentTask = NULL;
    thread->rsp = NULL;
    thread->next = NULL;
    thread->prev = NULL;
    if (parentTask) {
        PsTaskDetachThread(parentTask, thread);
        ObRelease(&parentTask->object);
    }
}

static void PsThreadStart(void) {
    PsThread *thread = PsCurrentThread();
    if (!thread) {
        __builtin_trap();
    }

    KeTssSwitchToRing3(thread->rip, thread->userStack);
    __builtin_unreachable();
}

PsThread *PsCreateThread(PsTask *parentTask, void *entryPoint, void *userStack) {
    if (!parentTask || !parentTask->pageTable || !entryPoint || !userStack) {
        return NULL;
    }

    PsThreadCreateData createData = {
        .parentTask = parentTask,
        .entryPoint = (uintptr_t)entryPoint,
        .userStack = (uintptr_t)userStack,
    };
    return (PsThread *)ObCreateInPool(&gThreadPool, ObObjectTypeThread, &createData);
}
