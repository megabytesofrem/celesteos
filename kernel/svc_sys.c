#include "svc_sys.h"

#include <ipc/endpoint.h>
#include <ipc/scheduler.h>
#include <ipc/task.h>
#include <ipc/thread.h>

#include <error.h>
#include <klog.h>
#include <string.h>

int sys_svc_send_msg(handle_t target, svc_message_t *message, svc_message_t *recv_buffer) {
    // Resolve the handle
    task_t *curr_task = task_current();
    thread_t *curr_thread = thread_current();

    if (!curr_task || !curr_thread) {
        return ERR_INVALID_ARG;
    }

    // Lookup a task handle with the required rights (CAP_RIGHT_CALL), if we find it:
    // great, if we don't find it, we return an error.
    kobject_t *kobj = task_lookup_handle(curr_task, target, CAP_RIGHT_CALL);
    if (!kobj) {
        return ERR_INVALID_HANDLE;
    }

    // Ensure the object is an endpoint
    if (kobj->type != KOBJ_TYPE_ENDPOINT) {
        kobject_release(kobj);
        return ERR_INVALID_TYPE;
    }

    endpoint_t *ep = (endpoint_t *)kobj;
    spinlock_acquire(&ep->lock);

    curr_thread->ipc_recv_buf = recv_buffer;

    // Rendezvous check: is there a receiver waiting for a message?

    // If there is a receiver waiting, we can perform an immediate rendezvous, copy our payload,
    // link both TIDs and switch toe the server thread immediately, bypassing the ready queue.
    if (!wait_queue_is_empty(&ep->recv_queue)) {
        thread_t *server_thread = wait_queue_pop(&ep->recv_queue);

        // Direct memcpy into server threads msg payload
        if (message) {
            void *dest = server_thread->ipc_recv_buf ? server_thread->ipc_recv_buf
                                                     : server_thread->ipc_msg_payload;
            memcpy(dest, message, sizeof(svc_message_t));
        }

        // Set partner TIDs so both threads know who they are communicating with
        server_thread->ipc_partner_tid = curr_thread->tid;
        curr_thread->ipc_partner_tid = server_thread->tid;

        curr_thread->state = THREAD_BLOCKED_SEND;
        server_thread->state = THREAD_READY;

        spinlock_release(&ep->lock);
        kobject_release(kobj);

        // IPC fast pass: directly switch to the server thread
        scheduler_switch_to(server_thread);

        // When execution resumes here, the server replied and the IPC transaction is complete.
        return 0;
    }

    // If no receiver is waiting, we need to block the current thread and enqueue it into the send
    // queue.
    if (message) {
        memcpy(curr_thread->ipc_msg_payload, message, sizeof(svc_message_t));
    }

    curr_thread->state = THREAD_BLOCKED_SEND;
    wait_queue_enqueue(&ep->send_queue, curr_thread);

    spinlock_release(&ep->lock);
    kobject_release(kobj);

    // Yield the CPU to allow other threads to run, until a server dequeues this thread
    // in sys_svc_reply_wait().
    scheduler_yield();

    return 0;
}

int sys_svc_reply_wait(handle_t client, svc_message_t *reply, svc_message_t *recv_buffer) {
    task_t *curr_task = task_current();
    thread_t *curr_thread = thread_current();

    if (!curr_task || !curr_thread) {
        return ERR_INVALID_ARG;
    }

    // ----------------------------------------------------------------------
    // Step 1: Reply the client handle if a valid client handle is provided.
    // ----------------------------------------------------------------------
    if (client != INVALID_HANDLE) {
        kobject_t *client_kobj = task_lookup_handle(curr_task, client, CAP_RIGHT_CALL);
        if (client_kobj && client_kobj->type == KOBJ_TYPE_THREAD) {
            thread_t *client_thread = (thread_t *)client_kobj;

            // Direct copy into the client's IPC message payload buffer
            if (reply) {
                void *dest = client_thread->ipc_recv_buf ? client_thread->ipc_recv_buf
                                                         : client_thread->ipc_msg_payload;
                memcpy(dest, reply, sizeof(svc_message_t));
            }

            // Unblock client and wake it up if it was waiting for a reply.
            client_thread->state = THREAD_READY;
            scheduler_enqueue(client_thread);
            kobject_release(client_kobj);
        }
    }

    // ----------------------------------------------------------------------
    // Step 2: Wait for (and receive) a message from a server.
    // ----------------------------------------------------------------------

    // Lookup the endpoint associated with the current task (handle slot 0 assigned to server)
    handle_t ep_handle = 0;
    kobject_t *ep_kobj = task_lookup_handle(curr_task, ep_handle, CAP_RIGHT_CALL);

    if (!ep_kobj || ep_kobj->type != KOBJ_TYPE_ENDPOINT) {
        if (ep_kobj) {
            kobject_release(ep_kobj);
        }
        return ERR_INVALID_HANDLE;
    }

    endpoint_t *ep = (endpoint_t *)ep_kobj;
    spinlock_acquire(&ep->lock);

    curr_thread->ipc_recv_buf = recv_buffer;

    // Fast-Path: A client is already waiting on the send queue
    if (!wait_queue_is_empty(&ep->send_queue)) {
        thread_t *client_thread = wait_queue_pop(&ep->send_queue);

        // Direct payload transfer from client to server
        if (recv_buffer) {
            memcpy(recv_buffer, client_thread->ipc_msg_payload, sizeof(svc_message_t));
        }

        curr_thread->ipc_partner_tid = client_thread->tid;

        spinlock_release(&ep->lock);
        kobject_release(ep_kobj);

        return 0;
    }

    // Fallback: No client is waiting, park server on recv_queue
    curr_thread->state = THREAD_BLOCKED_RECV;
    wait_queue_enqueue(&ep->recv_queue, curr_thread);

    spinlock_release(&ep->lock);
    kobject_release(ep_kobj);

    // Block server until a client calls sys_svc_send_msg
    scheduler_yield();

    // Woken up by client: copy the payload deposited into local thread struct
    if (recv_buffer) {
        memcpy(recv_buffer, curr_thread->ipc_msg_payload, sizeof(svc_message_t));
    }

    return 0;
}

int sys_svc_vm_map(handle_t target, void *virt_addr, void *phys_addr, int flags) {
    // Implementation goes here
    (void)virt_addr;
    (void)phys_addr;
    (void)flags;

    klog(KLOG_INFO, "svc: Mapping memory for target: %d", target);
    return 0;
}

int sys_svc_vm_unmap(handle_t target, void *virt_addr) {
    // Implementation goes here
    (void)virt_addr;

    klog(KLOG_INFO, "svc: Unmapping memory for target: %d", target);
    return 0;
}

int sys_svc_thread_create(handle_t target, void *entry_point, void *user_stack_top) {
    // Implementation goes here
    (void)entry_point;
    (void)user_stack_top;

    klog(KLOG_INFO, "svc: Creating thread for target: %d", target);
    return 0;
}

int sys_svc_thread_yield(handle_t target) {
    // Implementation goes here
    klog(KLOG_INFO, "svc: Yielding thread for target: %d", target);
    return 0;
}

int sys_svc_unknown(handle_t target) {
    // Implementation goes here
    klog(KLOG_INFO, "svc: Unknown syscall for target: %d", target);
    return 0;
}