#include <ps/endpoint.h>

#include <ob/obcreation.h>

#define MAX_ENDPOINTS 256

static PsEndpoint g_endpoints[MAX_ENDPOINTS];
static ObObjectPool g_endpointPool;

static void PsCreateEndpointObject(ObHeader *object, ObObjectData data);
static void PsDestroyEndpointObject(ObHeader *object);

static const ObOperations g_endpointOperations = {
    .name = "endpoint",
    .create = PsCreateEndpointObject,
    .destroy = PsDestroyEndpointObject,
};

void PsEndpointSubsystemInit(void) {
    ObObjectPoolInit(
        &g_endpointPool, g_endpoints, sizeof(g_endpoints[0]), MAX_ENDPOINTS, &g_endpointOperations);
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
    return (PsEndpoint *)ObCreateInPool(&g_endpointPool, ObObjectTypeEndpoint, NULL);
}
