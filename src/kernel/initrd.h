#ifndef CELESTEOS_INITRD_H
#define CELESTEOS_INITRD_H

#include <stddef.h>
#include <stdint.h>

#include <mach/kern_return.h>

#define INITRD_TAR_REGULAR_FILE '0'
#define INITRD_TAR_AREGULAR_FILE '\0'
#define INITRD_TAR_DIRECTORY '5'
#define INITRD_TAR_BLOCK_SIZE 512

typedef struct __attribute__((packed)) initrd_tar_header {
    char name[100];
    char mode[8];
    char uid[8];
    char gid[8];
    char size[12];
    char mtime[12];
    char chksum[8];
    char typeflag;
    char linkname[100];
    char magic[6];
    char version[2];
    char uname[32];
    char gname[32];
    char devmajor[8];
    char devminor[8];
    char prefix[155];
    char padding[12];
} initrd_tar_header_t;

typedef struct initrd_tar_archive {
    void *base;
    size_t size;
} initrd_tar_archive_t;

typedef struct initrd_directory {
    char *name;
    struct initrd_directory **subdirectories;
    struct initrd_file **files;
    size_t subdirectory_count;
    size_t file_count;
} initrd_directory_t;

/**
 * A file within the initial ramdisk filesystem.
 */
typedef struct initrd_file {
    char *name;
    void *data;

    /* TODO: Add file permissions, timestamps, and other metadata */

    size_t size;
} initrd_file_t;

/**
 * An initial ramdisk filesystem structure, backed by a tar archive.
 */
typedef struct initrd_filesystem {
    struct initrd_directory *root;
} initrd_filesystem_t;

/**
 * Initialize a initial ramdisk filesystem with the given root directory.
 */
kern_return_t initrd_init_fs(struct initrd_filesystem *fs, struct initrd_directory *root,
                             size_t file_count);

/**
 * Load a tar archive into the initial ramdisk filesystem.
 */
kern_return_t initrd_load_tar(struct initrd_filesystem *fs, const initrd_tar_archive_t *archive);

/**
 * Destroy an initial ramdisk filesystem, freeing its resources.
 */
kern_return_t initrd_destroy(struct initrd_filesystem *fs);

/**
 * Create a new file in the initial ramdisk filesystem.
 */
kern_return_t initrd_create_file(struct initrd_filesystem *fs, const char *path, void *data,
                                 size_t size);

/**
 * Create a new directory in the initial ramdisk filesystem.
 */
kern_return_t initrd_create_directory(struct initrd_filesystem *fs, const char *name);

/**
 * Delete a file from the initial ramdisk filesystem by its path.
 */
kern_return_t initrd_delete_file(struct initrd_filesystem *fs, const char *path);

/**
 * Read a file from the initial ramdisk filesystem by its path.
 */
kern_return_t initrd_read_file(struct initrd_filesystem *fs, const char *path, void **data,
                               size_t *size);

#endif // CELESTEOS_INITRD_H