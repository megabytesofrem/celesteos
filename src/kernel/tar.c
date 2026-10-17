#include "tar.h"

#include <string.h>

static size_t octal_to_uint(const char *str, size_t size) {
    size_t value = 0;
    for (size_t i = 0; i < size && str[i] >= '0' && str[i] <= '7'; i++) {
        value = (value << 3) + (str[i] - '0');
    }
    return value;
}

kern_return_t
tar_lookup_file(const tar_archive_t *archive, const char *filename, tar_file_t *out_file) {
    if (!archive || !filename || !out_file) {
        return KERN_INVALID_ARGUMENT;
    }

    const char *rel_path = (filename[0] == '/') ? filename + 1 : filename;

    uint8_t *file_data = (uint8_t *)archive->base;
    uint8_t *archive_end = file_data + archive->size;

    while (file_data + TAR_BLOCK_SIZE <= archive_end) {
        tar_header_t *header = (tar_header_t *)file_data;
        if (header->name[0] == '\0') {
            break; // End of archive
        }

        // Parse the file size from the header
        size_t file_size = octal_to_uint(header->size, sizeof(header->size));
        void *file_payload = (void *)(file_data + TAR_BLOCK_SIZE);

        // tar archives commonly store paths with a leading "./".
        const char *header_path = header->name;
        while (header_path[0] == '.' && header_path[1] == '/') {
            header_path += 2;
        }

        // Check if the current header matches the requested filename
        if (strncmp(header_path, rel_path, sizeof(header->name)) == 0) {
            out_file->base = file_data + TAR_BLOCK_SIZE;
            out_file->size = file_size;

            return KERN_SUCCESS;
        }

        // Move to the next file header in the archive
        size_t blocks = (file_size + (TAR_BLOCK_SIZE - 1)) / TAR_BLOCK_SIZE;

        // Advance the pointer by the size of the current file's data, rounded up to the nearest block,
        // plus the header size
        file_data += TAR_BLOCK_SIZE + (blocks * TAR_BLOCK_SIZE);
    }

    return KERN_NOT_FOUND;
}