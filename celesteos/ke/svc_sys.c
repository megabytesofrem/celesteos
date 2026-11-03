#include "svc_sys.h"

#include <error.h>
#include <ob/obcreation.h>
#include <ps/endpoint.h>
#include <ps/pstask.h>
#include <ps/psthread.h>
#include <ps/sched.h>

#include <string.h>

static void KeSvcCopyMessage(SvcMessage *destination, const SvcMessage *source) {
    if (destination) {
        memcpy(destination, source, sizeof(*destination));
    }
}

static KeSvcHandle KeSvcCreateClientHandle(PsThread *serverThread, PsThread *clientThread) {
    return PsInsertTaskHandle(serverThread->parentTask, &clientThread->object, PS_ACCESS_CALL);
}

int KeSvcSendMessage(KeSvcHandle target, SvcMessage *message, SvcMessage *recvBuffer) {
    // Resolve the handle
    PsTask *currentTask = PsCurrentTask();
    PsThread *currentThread = PsCurrentThread();
    if (!currentTask || !currentThread || !message) {
        return ERR_INVALID_ARG;
    }

    // Lookup a task handle with the required rights (PS_ACCESS_CALL), if we find it:
    // great, if we don't find it, we return an error.
    ObHeader *object = PsGetTaskHandleObject(currentTask, target, PS_ACCESS_CALL);
    if (!object) {
        return ERR_INVALID_HANDLE;
    }
    // Ensure the object is an endpoint
    if (object->type != ObObjectTypeEndpoint) {
        ObRelease(object);
        return ERR_INVALID_TYPE;
    }

    PsEndpoint *endpoint = (PsEndpoint *)object;
    KeSpinLockAcquire(&endpoint->lock);
    currentThread->ipcReceiveBuffer = recvBuffer;

    // Rendezvous check: is there a receiver waiting for a message?
    PsThread *serverThread = PsWaitQueuePop(&endpoint->receiveQueue);
    if (serverThread) {
        KeSvcHandle clientHandle = KeSvcCreateClientHandle(serverThread, currentThread);
        if (clientHandle != INVALID_HANDLE) {
            // If there is a receiver waiting, perform an immediate rendezvous and switch
            // directly to the server thread, bypassing the ready queue.
            // Direct copy into the server thread's IPC message payload.
            KeSvcCopyMessage(&serverThread->ipcMessage, message);

            // Set partner IDs so both threads know who they are communicating with.
            serverThread->ipcPartnerId = currentThread->tid;
            currentThread->ipcPartnerId = serverThread->tid;
            serverThread->ipcMessage.registers[0] = (uintptr_t)clientHandle;

            currentThread->state = PsThreadStateBlockedSend;
            serverThread->state = PsThreadStateReady;
            KeSpinLockRelease(&endpoint->lock);
            ObRelease(object);

            // IPC fast pass: directly switch to the server thread.
            PsSchedulerSwitchTo(serverThread);
            PsSchedulerSwitchTo(serverThread);
            currentThread->ipcReceiveBuffer = NULL;
            // When execution resumes here, the server replied and the IPC transaction is complete.
            return 0;
        }
        PsWaitQueueEnqueue(&endpoint->receiveQueue, serverThread);
    }

    // If no receiver is waiting, block the current thread and enqueue it into the send queue.
    currentThread->ipcMessage = *message;
    currentThread->state = PsThreadStateBlockedSend;
    PsWaitQueueEnqueue(&endpoint->sendQueue, currentThread);
    KeSpinLockRelease(&endpoint->lock);
    ObRelease(object);

    // Yield the CPU to allow other threads to run, until a server dequeues this thread
    // in KeSvcReplyWait().
    PsSchedulerYield();
    KeSvcCopyMessage(recvBuffer, &currentThread->ipcMessage);
    currentThread->ipcReceiveBuffer = NULL;
    return 0;
}

int KeSvcReplyWait(KeSvcHandle client, SvcMessage *reply, SvcMessage *recvBuffer) {
    PsTask *currentTask = PsCurrentTask();
    PsThread *currentThread = PsCurrentThread();
    if (!currentTask || !currentThread) {
        return ERR_INVALID_ARG;
    }

    // ----------------------------------------------------------------------
    // Step 1: Reply to the client handle if a valid client handle is provided.
    // ----------------------------------------------------------------------
    if (client != INVALID_HANDLE) {
        ObHeader *clientObject = PsGetTaskHandleObject(currentTask, client, PS_ACCESS_CALL);
        if (!clientObject) {
            return ERR_INVALID_HANDLE;
        }
        if (clientObject->type != ObObjectTypeThread) {
            ObRelease(clientObject);
            return ERR_INVALID_TYPE;
        }

        PsThread *clientThread = (PsThread *)clientObject;
        // Direct copy into the client's IPC message payload buffer.
        if (reply) {
            KeSvcCopyMessage(&clientThread->ipcMessage, reply);
        }
        // Unblock the client and wake it up if it was waiting for a reply.
        PsSchedulerEnqueueThread(clientThread);
        ObRelease(clientObject);
        PsTaskReleaseHandle(currentTask, client);
    }

    // ----------------------------------------------------------------------
    // Step 2: Wait for (and receive) a message from a client.
    // ----------------------------------------------------------------------

    // Lookup the endpoint associated with the current task (handle slot 0 assigned to server).
    ObHeader *endpointObject = PsGetTaskHandleObject(currentTask, 0, PS_ACCESS_CALL);
    if (!endpointObject) {
        return ERR_INVALID_HANDLE;
    }
    if (endpointObject->type != ObObjectTypeEndpoint) {
        ObRelease(endpointObject);
        return ERR_INVALID_TYPE;
    }

    PsEndpoint *endpoint = (PsEndpoint *)endpointObject;
    KeSpinLockAcquire(&endpoint->lock);
    currentThread->ipcReceiveBuffer = recvBuffer;

    // Fast-Path: a client is already waiting on the send queue.
    PsThread *clientThread = PsWaitQueuePop(&endpoint->sendQueue);
    if (clientThread) {
        KeSvcHandle clientHandle = KeSvcCreateClientHandle(currentThread, clientThread);
        if (clientHandle == INVALID_HANDLE) {
            PsWaitQueueEnqueue(&endpoint->sendQueue, clientThread);
            KeSpinLockRelease(&endpoint->lock);
            ObRelease(endpointObject);
            return ERR_NO_MEMORY;
        }
        currentThread->ipcPartnerId = clientThread->tid;
        if (recvBuffer) {
            KeSvcCopyMessage(recvBuffer, &clientThread->ipcMessage);
            recvBuffer->registers[0] = (uintptr_t)clientHandle;
        } else {
            currentThread->ipcMessage = clientThread->ipcMessage;
            currentThread->ipcMessage.registers[0] = (uintptr_t)clientHandle;
        }
        KeSpinLockRelease(&endpoint->lock);
        ObRelease(endpointObject);
        return 0;
    }

    // Fallback: no client is waiting, so park the server on the receive queue.
    currentThread->state = PsThreadStateBlockedReceive;
    PsWaitQueueEnqueue(&endpoint->receiveQueue, currentThread);
    KeSpinLockRelease(&endpoint->lock);
    ObRelease(endpointObject);

    // Block the server until a client calls KeSvcSendMessage().
    PsSchedulerYield();
    // Woken up by client: copy the payload deposited into the local thread struct.
    KeSvcCopyMessage(recvBuffer, &currentThread->ipcMessage);
    currentThread->ipcReceiveBuffer = NULL;
    return 0;
}

int KeSvcVMMap(KeSvcHandle target, void *virtAddr, void *physAddr, int flags) {
    (void)target;
    (void)virtAddr;
    (void)physAddr;
    (void)flags;
    return ERR_INVALID_STATE;
}

int KeSvcVMUnmap(KeSvcHandle target, void *virtAddr) {
    (void)target;
    (void)virtAddr;
    return ERR_INVALID_STATE;
}

int KeSvcCreateThread(KeSvcHandle target, void *entryPoint, void *userStackTop) {
    if (!entryPoint || !userStackTop) {
        return ERR_INVALID_ARG;
    }

    PsTask *currentTask = PsCurrentTask();
    if (!currentTask) {
        return ERR_INVALID_STATE;
    }
    ObHeader *object = PsGetTaskHandleObject(currentTask, target, PS_ACCESS_WRITE);
    if (!object) {
        return ERR_INVALID_HANDLE;
    }
    if (object->type != ObObjectTypeTask) {
        ObRelease(object);
        return ERR_INVALID_TYPE;
    }

    PsThread *thread = PsCreateThread((PsTask *)object, entryPoint, userStackTop);
    ObRelease(object);
    if (!thread) {
        return ERR_NO_MEMORY;
    }
    PsSchedulerEnqueueThread(thread);
    ObRelease(&thread->object);
    return 0;
}

int KeSvcYieldThread(KeSvcHandle target) {
    if (target == INVALID_HANDLE) {
        PsSchedulerYield();
        return 0;
    }

    PsTask *currentTask = PsCurrentTask();
    if (!currentTask) {
        return ERR_INVALID_STATE;
    }
    ObHeader *object = PsGetTaskHandleObject(currentTask, target, PS_ACCESS_CALL);
    if (!object) {
        return ERR_INVALID_HANDLE;
    }
    if (object->type != ObObjectTypeThread) {
        ObRelease(object);
        return ERR_INVALID_TYPE;
    }

    PsThread *targetThread = (PsThread *)object;
    if (targetThread != PsCurrentThread() && targetThread->state != PsThreadStateReady) {
        ObRelease(object);
        return ERR_INVALID_STATE;
    }

    PsSchedulerSwitchTo(targetThread);
    ObRelease(object);
    return 0;
}

int KeSvcUnknownService(KeSvcHandle target) {
    (void)target;
    return ERR_NOT_FOUND;
}
