#ifndef CELESTEOS_WAIT_QUEUE_H
#define CELESTEOS_WAIT_QUEUE_H

#include <ps/psthread.h>

#include <stdbool.h>

typedef struct PsWaitQueue {
    PsThread *head;
    PsThread *tail;
} PsWaitQueue;

static inline void PsWaitQueueInit(PsWaitQueue *queue) {
    if (queue) {
        queue->head = NULL;
        queue->tail = NULL;
    }
}

static inline bool PsWaitQueueIsEmpty(const PsWaitQueue *queue) {
    return !queue || queue->head == NULL;
}

static inline void PsWaitQueueEnqueue(PsWaitQueue *queue, PsThread *thread) {
    if (!queue || !thread) {
        return;
    }

    thread->next = NULL;
    thread->prev = queue->tail;
    if (queue->tail) {
        queue->tail->next = thread;
    } else {
        queue->head = thread;
    }
    queue->tail = thread;
}

static inline void PsWaitQueueDequeue(PsWaitQueue *queue, PsThread *thread) {
    if (!queue || !thread) {
        return;
    }

    if (thread->prev) {
        thread->prev->next = thread->next;
    } else if (queue->head == thread) {
        queue->head = thread->next;
    } else {
        return;
    }
    if (thread->next) {
        thread->next->prev = thread->prev;
    } else {
        queue->tail = thread->prev;
    }
    thread->next = NULL;
    thread->prev = NULL;
}

static inline PsThread *PsWaitQueuePeek(const PsWaitQueue *queue) {
    return queue ? queue->head : NULL;
}

static inline PsThread *PsWaitQueuePop(PsWaitQueue *queue) {
    PsThread *thread = PsWaitQueuePeek(queue);
    if (thread) {
        // Dequeue the head thread from the queue
        PsWaitQueueDequeue(queue, thread);
    }
    return thread;
}

static inline void PsWaitQueueClear(PsWaitQueue *queue) {
    while (!PsWaitQueueIsEmpty(queue)) {
        PsWaitQueuePop(queue);
    }
}

#endif // CELESTEOS_WAIT_QUEUE_H
