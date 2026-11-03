#include <ps/endpoint.h>

#include <ob/obcreation.h>

#define MAX_ENDPOINTS 256

static PsEndpoint gEndpoints[MAX_ENDPOINTS];
static ObObjectPool gEndpointPool;

static void PsCreateEndpointObject(ObHeader *object, ObObjectData data);
static void PsDestroyEndpointObject(ObHeader *object);

static const ObOperations gEndpointOperations = {
    .name = "endpoint",
    .create = PsCreateEndpointObject,
    .destroy = PsDestroyEndpointObject,
};

void PsEndpointSubsystemInit(void) {
    ObObjectPoolInit(&gEndpointPool, gEndpoints, sizeof(gEndpoints[0]), MAX_ENDPOINTS,
        &gEndpointOperations);
}

void PsCreateEndpoint(PsEndpoint *endpoint, ObObjectData data) {
    (void)data;
    // Initialize the send and receive queues
    KeSpinLockInit(&endpoint->lock);
    PsWaitQueueInit(&endpoint->sendQueue);
    PsWaitQueueInit(&endpoint->receiveQueue);
}

void PsDestroyEndpoint(PsEndpoint *endpoint) {
    // Clean up the send and receive queues
    PsWaitQueueClear(&endpoint->sendQueue);
    PsWaitQueueClear(&endpoint->receiveQueue);
}

static void PsCreateEndpointObject(ObHeader *object, ObObjectData data) {
    PsCreateEndpoint((PsEndpoint *)object, data);
}

static void PsDestroyEndpointObject(ObHeader *object) {
    PsDestroyEndpoint((PsEndpoint *)object);
}

PsEndpoint *PsEndpointCreate(void) {
    return (PsEndpoint *)ObCreateInPool(&gEndpointPool, ObObjectTypeEndpoint, NULL);
}
