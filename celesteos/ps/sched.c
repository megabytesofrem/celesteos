#include <ps/sched.h>

#include <ke/page.h>
#include <ke/tss.h>
#include <ob/obcreation.h>
#include <ps/pstask.h>
#include <ps/wait_queue.h>

static PsThread *gCurrentThread;
static PsWaitQueue gReadyQueue;
static uint64_t *gBootRsp;

static void PsSchedulerPerformSwitch(PsThread *previousThread, PsThread *nextThread);

void PsSchedulerInit(void) {
    gCurrentThread = NULL;
    gBootRsp = NULL;
    PsWaitQueueInit(&gReadyQueue);
}

PsThread *PsSchedulerCurrentThread(void) {
    return gCurrentThread;
}

void PsSchedulerEnqueueThread(PsThread *thread) {
    if (!thread || thread == gCurrentThread || thread->state == PsThreadStateReady) {
        return;
    }

    thread->state = PsThreadStateReady;
    if (!thread->schedulerReferenceHeld) {
        ObRetain(&thread->object);
        thread->schedulerReferenceHeld = true;
    }
    PsWaitQueueEnqueue(&gReadyQueue, thread);
}

void PsSchedulerDequeueThread(PsThread *thread) {
    if (!thread || thread->state != PsThreadStateReady) {
        return;
    }

    PsWaitQueueDequeue(&gReadyQueue, thread);
    thread->state = PsThreadStateInactive;
    if (thread->schedulerReferenceHeld) {
        thread->schedulerReferenceHeld = false;
        ObRelease(&thread->object);
    }
}

void PsSchedulerSchedule(void) {
    PsThread *nextThread = PsWaitQueuePop(&gReadyQueue);
    if (!nextThread) {
        return;
    }

    PsThread *previousThread = gCurrentThread;
    if (previousThread && previousThread->state == PsThreadStateRunning) {
        previousThread->state = PsThreadStateReady;
        PsWaitQueueEnqueue(&gReadyQueue, previousThread);
    }
    PsSchedulerPerformSwitch(previousThread, nextThread);
}

void PsSchedulerSwitchTo(PsThread *thread) {
    if (!thread || thread == gCurrentThread) {
        return;
    }

    if (thread->state == PsThreadStateReady) {
        PsWaitQueueDequeue(&gReadyQueue, thread);
    }

    PsThread *previousThread = gCurrentThread;
    if (previousThread && previousThread->state == PsThreadStateRunning) {
        previousThread->state = PsThreadStateReady;
        PsWaitQueueEnqueue(&gReadyQueue, previousThread);
    }
    PsSchedulerPerformSwitch(previousThread, thread);
}

void PsSchedulerYield(void) {
    PsThread *currentThread = gCurrentThread;
    if (!currentThread) {
        return;
    }

    while (currentThread->state != PsThreadStateRunning) {
        if (PsWaitQueueIsEmpty(&gReadyQueue)) {
            __asm__ volatile("sti; hlt" : : : "memory");
            continue;
        }
        PsSchedulerSchedule();
    }

    if (!PsWaitQueueIsEmpty(&gReadyQueue)) {
        PsSchedulerSchedule();
    }
}

static void PsSchedulerPerformSwitch(PsThread *previousThread, PsThread *nextThread) {
    if (!nextThread || previousThread == nextThread) {
        return;
    }

    gCurrentThread = nextThread;
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

    uint64_t **savedRsp = previousThread ? &previousThread->rsp : &gBootRsp;
    ctx_switch(savedRsp, nextThread->rsp);
}