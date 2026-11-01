/**
 * File: endpoint.h
 *
 * Purpose: Defines the IPC endpoint object and its operations.
 */

#ifndef CELESTEOS_ENDPOINT_H
#define CELESTEOS_ENDPOINT_H

#include "kobject.h"
#include "wait_queue.h"

#include <kernel/spinlock.h>

typedef struct endpoint {
    kobject_t kobj;
    spinlock_t lock;

    wait_queue_t send_queue;
    wait_queue_t recv_queue;
} endpoint_t;

/**
 * Create an IPC endpoint object.
 *
 * This function initializes the IPC endpoint, setting up its send and receive wait queues.
 */
void endpoint_create(kobject_t *kobj, kobject_data_t data);

/**
 * Destroy an IPC endpoint object.
 *
 * This function clears the send and receive wait queues associated with the endpoint.
 */
void endpoint_destroy(kobject_t *kobj);

#endif // CELESTEOS_ENDPOINT_H