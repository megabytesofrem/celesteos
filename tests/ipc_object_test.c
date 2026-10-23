#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <kernel/ipc/ipc_port.h>
#include <kernel/ipc/ipc_space.h>
#include <kernel/ipc/mach_message.h>
#include <kernel/ipc/mach_traps.h>
#include <kernel/ipc/task.h>
#include <kernel/ipc/thread.h>
#include <kernel/memory/pmap.h>
#include <klog.h>

thread_t *g_current_thread;

void *kmalloc(size_t size) {
    return malloc(size);
}

void kfree(void *ptr) {
    free(ptr);
}

void klog(enum log_level level, const char *format, ...) {
    (void)level;
    (void)format;
}

int pmap_copy_from_user(void *destination, uintptr_t source, size_t size) {
    if (destination == NULL || (size != 0 && source == 0)) {
        return -EINVAL;
    }
    memcpy(destination, (const void *)source, size);
    return 0;
}

int pmap_copy_to_user(uintptr_t destination, const void *source, size_t size) {
    if (source == NULL || (size != 0 && destination == 0)) {
        return -EINVAL;
    }
    memcpy((void *)destination, source, size);
    return 0;
}

int main(void) {
    task_t task = { .itk_self = 77 };
    thread_t thread = { .parent_task = &task, .itk_self = 88 };
    g_current_thread = &thread;
    assert(ipc_space_create(&task.itk_space) == KERN_SUCCESS);
    assert(task_self_trap() == 77);
    assert(thread_self_trap() == 88);

    mach_port_name_t receive_name = MACH_PORT_NULL;
    assert(mach_port_allocate_trap(MACH_PORT_RIGHT_RECEIVE, &receive_name) == KERN_SUCCESS);
    mach_port_name_t send_name;
    assert(mach_port_make_send_trap(receive_name, &send_name) == KERN_SUCCESS);

    struct {
        mach_msg_header_t header;
        uint64_t payload;
    } sent = {
        .header = {
            .msgh_bits = 0,
            .msgh_size = sizeof(sent),
            .msgh_remote_port = send_name,
            .msgh_local_port = MACH_PORT_NULL,
            .msgh_id = 42,
        },
        .payload = UINT64_C(0xC0FFEE),
    };
    assert(mach_msg_trap(&sent.header, MACH_SEND_MSG, sizeof(sent), 0, receive_name, 0, 0) ==
           MACH_MSG_SUCCESS);

    struct {
        mach_msg_header_t header;
        uint64_t payload;
    } received = { 0 };
    assert(mach_msg_receive(&received.header, MACH_RCV_MSG, sizeof(received.header), receive_name,
                            0) == MACH_RCV_TOO_LARGE);
    assert(mach_msg_receive(&received.header, MACH_RCV_MSG, sizeof(received), receive_name, 0) ==
           MACH_MSG_SUCCESS);
    assert(received.header.msgh_id == 42);
    assert(received.payload == UINT64_C(0xC0FFEE));
    assert(mach_msg_receive(&received.header, MACH_RCV_MSG, sizeof(received), receive_name, 0) ==
           MACH_RCV_TIMED_OUT);

    sent.header.msgh_remote_port = receive_name;
    assert(mach_msg_send(&sent.header, MACH_SEND_MSG, sizeof(sent), 0) == MACH_SEND_INVALID_RIGHT);

    assert(mach_port_deallocate_trap(send_name) == KERN_SUCCESS);
    assert(mach_port_deallocate_trap(receive_name) == KERN_SUCCESS);
    assert(ipc_space_destroy(task.itk_space) == KERN_SUCCESS);
    return 0;
}
