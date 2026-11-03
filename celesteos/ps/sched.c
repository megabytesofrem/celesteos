#include <ps/sched.h>

#include <ke/page.h>
#include <ke/tss.h>
#include <ob/obcreation.h>
#include <ps/pstask.h>
#include <ps/wait_queue.h>

static PsThread *g_currentThread;
static PsWaitQueue g_readyQueue;
static uint64_t *g_bootRsp;

static void PsSchedulerPerformSwitch(PsThread *previousThread, PsThread *nextThread);

void PsSchedulerInit(void) {
    g_currentThread = NULL;
    g_bootRsp = NULL;
    PsWaitQueueInit(&g_readyQueue);
}

PsThread *PsSchedulerCurrentThread(void) {
    return g_currentThread;
}

void PsSchedulerEnqueueThread(PsThread *thread) {
    if (!thread || thread == g_currentThread || thread->state == PsThreadStateReady) {
        return;
    }

    thread->state = PsThreadStateReady;
    if (!thread->schedulerReferenceHeld) {
        ObRetain(&thread->object);
        thread->schedulerReferenceHeld = true;
    }
    PsWaitQueueEnqueue(&g_readyQueue, thread);
}

void PsSchedulerDequeueThread(PsThread *thread) {
    if (!thread || thread->state != PsThreadStateReady) {
        return;
    }

    PsWaitQueueDequeue(&g_readyQueue, thread);
    thread->state = PsThreadStateInactive;
    if (thread->schedulerReferenceHeld) {
        thread->schedulerReferenceHeld = false;
        ObRelease(&thread->object);
    }
}

void PsSchedulerSchedule(void) {
    PsThread *nextThread = PsWaitQueuePop(&g_readyQueue);
    if (!nextThread) {
        return;
    }

    PsThread *previousThread = g_currentThread;
    if (previousThread && previousThread->state == PsThreadStateRunning) {
        previousThread->state = PsThreadStateReady;
        PsWaitQueueEnqueue(&g_readyQueue, previousThread);
    }
    PsSchedulerPerformSwitch(previousThread, nextThread);
}

void PsSchedulerSwitchTo(PsThread *thread) {
    if (!thread || thread == g_currentThread) {
        return;
    }

    if (thread->state == PsThreadStateReady) {
        PsWaitQueueDequeue(&g_readyQueue, thread);
    }

    PsThread *previousThread = g_currentThread;
    if (previousThread && previousThread->state == PsThreadStateRunning) {
        previousThread->state = PsThreadStateReady;
        PsWaitQueueEnqueue(&g_readyQueue, previousThread);
    }
    PsSchedulerPerformSwitch(previousThread, thread);
}

void PsSchedulerYield(void) {
    PsThread *currentThread = g_currentThread;
    if (!currentThread) {
        return;
    }

    while (currentThread->state != PsThreadStateRunning) {
        if (PsWaitQueueIsEmpty(&g_readyQueue)) {
            __asm__ volatile("sti; hlt" : : : "memory");
            continue;
        }
        PsSchedulerSchedule();
    }

    if (!PsWaitQueueIsEmpty(&g_readyQueue)) {
        PsSchedulerSchedule();
    }
}

static void PsSchedulerPerformSwitch(PsThread *previousThread, PsThread *nextThread) {
    if (!nextThread || previousThread == nextThread) {
        return;
    }

    g_currentThread = nextThread;
    nextThread->state = PsThreadStateRunning;

    if (KeTssSetKernelStack(
            (uintptr_t)(nextThread->kernelStack + sizeof(nextThread->kernelStack))) != 0) {
        __builtin_trap();
    }

    PsTask *previousTask = previousThread ? previousThread->parentTask : NULL;
    PsTask *nextTask = nextThread->parentTask;
    if (nextTask && previousTask != nextTask && KePageMapSwitchTo(nextTask->pageTable) != 0) {
        __builtin_trap();
    }

    uint64_t **savedRsp = previousThread ? &previousThread->rsp : &g_bootRsp;
    ctx_switch(savedRsp, nextThread->rsp);
}