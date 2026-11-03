#ifndef CELESTEOS_PSENDPOINT_H
#define CELESTEOS_PSENDPOINT_H

#include <ke/spinlock.h>
#include <ke/svc.h>
#include <ob/obheader.h>
#include <ps/wait_queue.h>

typedef struct PsEndpoint {
    /* Common header for all objects */
    ObHeader object;
    KeSpinLock lock;
    PsWaitQueue sendQueue;
    PsWaitQueue receiveQueue;
} PsEndpoint;

/**
 * Initialize the endpoint object pool.
 */
void PsEndpointSubsystemInit(void);

/**
 * Create an IPC endpoint object and initialize its send and receive queues.
 */
void PsCreateEndpoint(PsEndpoint *endpoint, ObObjectData data);

/**
 * Destroy an IPC endpoint object and clean up its send and receive queues.
 */
void PsDestroyEndpoint(PsEndpoint *endpoint);

/**
 * Allocate a new IPC endpoint from the endpoint pool.
 */
PsEndpoint *PsEndpointCreate(void);

#endif // CELESTEOS_PSENDPOINT_H
