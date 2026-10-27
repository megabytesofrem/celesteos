/**
 * File: wait_queue.h
 *
 * Purpose: Defines a wait queue for managing threads that are waiting for events.
 */

#ifndef CELESTEOS_WAIT_QUEUE_H
#define CELESTEOS_WAIT_QUEUE_H

#include "thread.h"

#include <stdbool.h>

typedef struct wait_queue {
    thread_t *head;
    thread_t *tail;
} wait_queue_t;

static inline void wait_queue_init(wait_queue_t *queue) {
    queue->head = NULL;
    queue->tail = NULL;
}

static inline bool wait_queue_is_empty(wait_queue_t *queue) {
    return queue->head == NULL;
}

static inline void wait_queue_enqueue(wait_queue_t *queue, thread_t *thread) {
    thread->next = NULL;
    thread->prev = queue->tail;
    if (queue->tail) {
        queue->tail->next = thread;
    } else {
        queue->head = thread;
    }
    queue->tail = thread;
}

static inline void wait_queue_dequeue(wait_queue_t *queue, thread_t *thread) {
    if (thread->prev) {
        thread->prev->next = thread->next;
    } else {
        queue->head = thread->next;
    }
    if (thread->next) {
        thread->next->prev = thread->prev;
    } else {
        queue->tail = thread->prev;
    }
    thread->next = NULL;
    thread->prev = NULL;
}

static inline thread_t *wait_queue_peek(wait_queue_t *queue) {
    return queue->head;
}

static inline thread_t *wait_queue_pop(wait_queue_t *queue) {
    if (wait_queue_is_empty(queue)) {
        return NULL;
    }

    // Dequeue whatever the head of the queue is
    thread_t *thread = queue->head;
    wait_queue_dequeue(queue, thread);
    return thread;
}

static inline void wait_queue_clear(wait_queue_t *queue) {
    while (!wait_queue_is_empty(queue)) {
        wait_queue_pop(queue);
    }
}

#endif // CELESTEOS_WAIT_QUEUE_H