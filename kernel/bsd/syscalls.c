#include <kernel/memory/pmap.h>
#include <kernel/stub.h>
#include <kernel/sys_enter.h>
#include <klog.h>

#include <errno.h>
#include <stdint.h>
#include <string.h>

#define BSD_WRITE_BUFFER_SIZE 128

void sys_exit(int rval) {
    (void)rval;
    FIXME("task teardown and scheduling not implemented");
}

intptr_t sys_read(int fd, void *buf, size_t nbyte) {
    if (fd != 0 || (nbyte != 0 && buf == NULL) || nbyte > INTPTR_MAX) {
        // Invalid file descriptor or buffer parameters
        return -EINVAL;
    }

    // Create a buffer to hold chunks of data copied from user space
    unsigned char buffer[BSD_WRITE_BUFFER_SIZE];
    size_t read = 0;
    uintptr_t user_address = (uintptr_t)buf;

    while (read < nbyte) {
        // Chunk the data into smaller sizes
        size_t chunk_size = nbyte - read;
        if (chunk_size > sizeof(buffer)) {
            chunk_size = sizeof(buffer);
        }

        // Check if the user address is valid
        if (user_address > UINTPTR_MAX) {
            return -EFAULT;
        }

        // Copy the read data to the user buffer
        if (pmap_copy_to_user(user_address + read, buffer, chunk_size) != 0) {
            return read == 0 ? -EFAULT : (intptr_t)read;
        }

        read += chunk_size;
    }

    // Return the total number of bytes read
    return (intptr_t)read;
}

intptr_t sys_write(int fd, const void *buf, size_t nbyte) {
    if ((fd != 1 && fd != 2) || (nbyte != 0 && buf == NULL) || nbyte > INTPTR_MAX) {
        // Invalid file descriptor or buffer parameters
        return -EINVAL;
    }

    // Create a buffer to hold chunks of data copied from user space
    unsigned char buffer[BSD_WRITE_BUFFER_SIZE];
    size_t read = 0;
    uintptr_t user_address = (uintptr_t)buf;

    while (read < nbyte) {
        size_t chunk_size = nbyte - read;
        if (chunk_size > sizeof(buffer)) {
            chunk_size = sizeof(buffer);
        }

        // TODO: Populate 'buffer' with data from hardware/file descriptor
        // For early testing stubs, you might zero or fill 'buffer' here:
        FIXME("populate buffer with data from hardware/file descriptor");
        memset(buffer, 0, chunk_size);

        // Copy kernel 'buffer' INTO user memory at offset (user_address + read)
        if (pmap_copy_to_user(user_address + read, buffer, chunk_size) != 0) {
            return read == 0 ? -EFAULT : (intptr_t)read;
        }

        read += chunk_size;
    }

    return (intptr_t)read;
}
