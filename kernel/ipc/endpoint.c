#include "endpoint.h"

static const kobject_ops_t endpoint_ops = {
    .create = endpoint_create,
    .destroy = endpoint_destroy,
};

void endpoint_create(kobject_t *kobj, kobject_data_t data) {
    endpoint_t *ep = (endpoint_t *)kobj;
    spinlock_init(&ep->lock);

    wait_queue_init(&ep->send_queue);
    wait_queue_init(&ep->recv_queue);
}

void endpoint_destroy(kobject_t *kobj) {
    endpoint_t *ep = (endpoint_t *)kobj;

    wait_queue_clear(&ep->send_queue);
    wait_queue_clear(&ep->recv_queue);
}