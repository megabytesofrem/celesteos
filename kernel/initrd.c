#include "initrd.h"

#include <kernel/kmalloc.h>
#include <libkernel.h>
#include <stdbool.h>
#include <string.h>

typedef struct {
    const char *start;
    size_t length;
} path_component_t;

static size_t bounded_length(const char *string, size_t max_length) {
    size_t length = 0;
    while (length < max_length && string[length] != '\0') {
        length++;
    }
    return length;
}

static bool bytes_equal(const char *left, const char *right, size_t length) {
    for (size_t i = 0; i < length; i++) {
        if (left[i] != right[i]) {
            return false;
        }
    }
    return true;
}

static bool next_path_component(const char **path, path_component_t *component) {
    while (**path == '/') {
        (*path)++;
    }

    if (**path == '\0') {
        return false;
    }

    component->start = *path;
    while (**path != '\0' && **path != '/') {
        (*path)++;
    }
    component->length = (size_t)(*path - component->start);
    return true;
}

static bool path_component_is(const path_component_t *component, const char *name) {
    return strlen(name) == component->length &&
           bytes_equal(component->start, name, component->length);
}

static bool path_has_parent_component(const char *path) {
    path_component_t component;
    while (next_path_component(&path, &component)) {
        if (path_component_is(&component, "..")) {
            return true;
        }
    }
    return false;
}

static struct initrd_directory *find_subdirectory(struct initrd_directory *directory,
                                                  const path_component_t *component) {
    for (size_t i = 0; i < directory->subdirectory_count; i++) {
        struct initrd_directory *subdirectory = directory->subdirectories[i];
        if (strlen(subdirectory->name) == component->length &&
            bytes_equal(subdirectory->name, component->start, component->length)) {
            return subdirectory;
        }
    }
    return NULL;
}

static bool contains_file(struct initrd_directory *directory, const path_component_t *component) {
    for (size_t i = 0; i < directory->file_count; i++) {
        if (strlen(directory->files[i]->name) == component->length &&
            bytes_equal(directory->files[i]->name, component->start, component->length)) {
            return true;
        }
    }
    return false;
}

static kern_return_t append_subdirectory(struct initrd_directory *parent,
                                         const path_component_t *component,
                                         struct initrd_directory **out_directory) {
    struct initrd_directory *existing = find_subdirectory(parent, component);
    if (existing) {
        *out_directory = existing;
        return KERN_SUCCESS;
    }
    if (contains_file(parent, component)) {
        return KERN_ALREADY_EXISTS;
    }

    struct initrd_directory *directory = kmalloc(sizeof(*directory));
    if (!directory) {
        return KERN_NO_MEMORY;
    }
    memset(directory, 0, sizeof(*directory));

    directory->name = kmalloc(component->length + 1);
    if (!directory->name) {
        kfree(directory);
        return KERN_NO_MEMORY;
    }
    memcpy(directory->name, component->start, component->length);
    directory->name[component->length] = '\0';

    if (parent->subdirectory_count == SIZE_MAX / sizeof(*parent->subdirectories)) {
        kfree(directory->name);
        kfree(directory);
        return KERN_NO_MEMORY;
    }
    size_t count = parent->subdirectory_count + 1;
    struct initrd_directory **subdirectories = kmalloc(count * sizeof(*subdirectories));
    if (!subdirectories) {
        kfree(directory->name);
        kfree(directory);
        return KERN_NO_MEMORY;
    }
    if (parent->subdirectory_count != 0) {
        memcpy(subdirectories, parent->subdirectories,
               parent->subdirectory_count * sizeof(*subdirectories));
    }
    subdirectories[parent->subdirectory_count] = directory;

    kfree(parent->subdirectories);
    parent->subdirectories = subdirectories;
    parent->subdirectory_count = count;
    *out_directory = directory;
    return KERN_SUCCESS;
}

static kern_return_t find_parent_directory(struct initrd_filesystem *fs, const char *path,
                                           bool create_missing, struct initrd_directory **parent,
                                           path_component_t *leaf) {
    if (!fs || !fs->root || !path || !parent || !leaf) {
        return KERN_INVALID_ARGUMENT;
    }

    const char *cursor = path;
    path_component_t current;
    bool has_current = false;
    struct initrd_directory *directory = fs->root;

    while (next_path_component(&cursor, &current)) {
        if (path_component_is(&current, ".")) {
            continue;
        }
        if (path_component_is(&current, "..")) {
            return KERN_INVALID_ARGUMENT;
        }

        path_component_t next;
        const char *lookahead = cursor;
        bool has_next = false;
        while (next_path_component(&lookahead, &next)) {
            if (!path_component_is(&next, ".")) {
                has_next = true;
                break;
            }
        }

        if (!has_next) {
            *parent = directory;
            *leaf = current;
            has_current = true;
            break;
        }

        struct initrd_directory *child = find_subdirectory(directory, &current);
        if (!child && create_missing) {
            kern_return_t result = append_subdirectory(directory, &current, &child);
            if (result != KERN_SUCCESS) {
                return result;
            }
        }
        if (!child) {
            return KERN_NOT_FOUND;
        }
        directory = child;
    }

    return has_current ? KERN_SUCCESS : KERN_INVALID_ARGUMENT;
}

static void destroy_directory(struct initrd_directory *directory) {
    for (size_t i = 0; i < directory->file_count; i++) {
        struct initrd_file *file = directory->files[i];
        kfree(file->data);
        kfree(file->name);
        kfree(file);
    }
    kfree(directory->files);

    for (size_t i = 0; i < directory->subdirectory_count; i++) {
        struct initrd_directory *subdirectory = directory->subdirectories[i];
        destroy_directory(subdirectory);
        kfree(subdirectory->name);
        kfree(subdirectory);
    }
    kfree(directory->subdirectories);
}

static bool parse_octal_size(const char *string, size_t length, size_t *value) {
    size_t parsed = 0;
    bool found_digit = false;
    for (size_t i = 0; i < length; i++) {
        if (string[i] < '0' || string[i] > '7') {
            break;
        }
        size_t digit = (size_t)(string[i] - '0');
        if (parsed > (SIZE_MAX - digit) / 8) {
            return false;
        }
        parsed = parsed * 8 + digit;
        found_digit = true;
    }
    *value = found_digit ? parsed : 0;
    return true;
}

/**
 * initrd code.
 */

kern_return_t initrd_init_fs(struct initrd_filesystem *fs, struct initrd_directory *root,
                             size_t file_count) {
    (void)file_count;
    if (!fs) {
        return KERN_INVALID_ARGUMENT;
    }

    if (!root) {
        root = kmalloc(sizeof(*root));
        if (!root) {
            return KERN_NO_MEMORY;
        }
        memset(root, 0, sizeof(*root));
    }

    fs->root = root;
    return KERN_SUCCESS;
}

kern_return_t initrd_load_tar(struct initrd_filesystem *fs, const initrd_tar_archive_t *archive) {
    if (!fs || !fs->root || !archive || (!archive->base && archive->size != 0)) {
        return KERN_INVALID_ARGUMENT;
    }

    const uint8_t *file_data = archive->base;
    size_t remaining = archive->size;
    while (remaining >= INITRD_TAR_BLOCK_SIZE) {
        const initrd_tar_header_t *header = (const initrd_tar_header_t *)file_data;
        if (header->name[0] == '\0') {
            break;
        }

        size_t file_size;
        if (!parse_octal_size(header->size, sizeof(header->size), &file_size) ||
            file_size > remaining - INITRD_TAR_BLOCK_SIZE) {
            return KERN_INVALID_ARGUMENT;
        }
        size_t data_blocks =
            file_size / INITRD_TAR_BLOCK_SIZE + (file_size % INITRD_TAR_BLOCK_SIZE != 0);
        if (data_blocks > (remaining - INITRD_TAR_BLOCK_SIZE) / INITRD_TAR_BLOCK_SIZE) {
            return KERN_INVALID_ARGUMENT;
        }

        char path[257];
        size_t prefix_length = bounded_length(header->prefix, sizeof(header->prefix));
        size_t name_length = bounded_length(header->name, sizeof(header->name));
        if (prefix_length != 0) {
            memcpy(path, header->prefix, prefix_length);
            path[prefix_length] = '/';
            memcpy(path + prefix_length + 1, header->name, name_length);
            path[prefix_length + 1 + name_length] = '\0';
        } else {
            memcpy(path, header->name, name_length);
            path[name_length] = '\0';
        }

        kern_return_t result = KERN_SUCCESS;
        if (header->typeflag == INITRD_TAR_DIRECTORY) {
            result = initrd_create_directory(fs, path);
        } else if (header->typeflag == INITRD_TAR_REGULAR_FILE ||
                   header->typeflag == INITRD_TAR_AREGULAR_FILE) {
            result = initrd_create_file(
                fs, path, file_size == 0 ? NULL : (void *)(file_data + INITRD_TAR_BLOCK_SIZE),
                file_size);
        }
        if (result != KERN_SUCCESS) {
            return result;
        }

        size_t advance = INITRD_TAR_BLOCK_SIZE + data_blocks * INITRD_TAR_BLOCK_SIZE;
        file_data += advance;
        remaining -= advance;
    }

    return KERN_SUCCESS;
}

kern_return_t initrd_destroy(struct initrd_filesystem *fs) {
    if (!fs) {
        return KERN_INVALID_ARGUMENT;
    }
    if (fs->root) {
        destroy_directory(fs->root);
        kfree(fs->root->name);
        kfree(fs->root);
        fs->root = NULL;
    }
    return KERN_SUCCESS;
}

kern_return_t initrd_create_file(struct initrd_filesystem *fs, const char *path, void *data,
                                 size_t size) {
    if (!fs || !fs->root || !path || path_has_parent_component(path) || (size != 0 && !data)) {
        return KERN_INVALID_ARGUMENT;
    }

    struct initrd_directory *parent;
    path_component_t leaf;
    kern_return_t result = find_parent_directory(fs, path, true, &parent, &leaf);
    if (result != KERN_SUCCESS) {
        return result;
    }
    if (find_subdirectory(parent, &leaf) || contains_file(parent, &leaf)) {
        return KERN_ALREADY_EXISTS;
    }

    struct initrd_file *file = kmalloc(sizeof(*file));
    if (!file) {
        return KERN_NO_MEMORY;
    }
    memset(file, 0, sizeof(*file));

    file->name = kmalloc(leaf.length + 1);
    if (!file->name) {
        kfree(file);
        return KERN_NO_MEMORY;
    }
    memcpy(file->name, leaf.start, leaf.length);
    file->name[leaf.length] = '\0';

    if (size != 0) {
        file->data = kmalloc(size);
        if (!file->data) {
            kfree(file->name);
            kfree(file);
            return KERN_NO_MEMORY;
        }
        memcpy(file->data, data, size);
    }
    file->size = size;

    if (parent->file_count == SIZE_MAX / sizeof(*parent->files)) {
        kfree(file->data);
        kfree(file->name);
        kfree(file);
        return KERN_NO_MEMORY;
    }
    size_t count = parent->file_count + 1;
    struct initrd_file **files = kmalloc(count * sizeof(*files));
    if (!files) {
        kfree(file->data);
        kfree(file->name);
        kfree(file);
        return KERN_NO_MEMORY;
    }
    if (parent->file_count != 0) {
        memcpy(files, parent->files, parent->file_count * sizeof(*files));
    }
    files[parent->file_count] = file;

    kfree(parent->files);
    parent->files = files;
    parent->file_count = count;
    return KERN_SUCCESS;
}

kern_return_t initrd_create_directory(struct initrd_filesystem *fs, const char *path) {
    if (!fs || !fs->root || !path || path[0] == '\0' || path_has_parent_component(path)) {
        return KERN_INVALID_ARGUMENT;
    }

    const char *cursor = path;
    path_component_t component;
    struct initrd_directory *directory = fs->root;

    while (next_path_component(&cursor, &component)) {
        if (path_component_is(&component, ".")) {
            continue;
        }
        if (path_component_is(&component, "..")) {
            return KERN_INVALID_ARGUMENT;
        }

        kern_return_t result = append_subdirectory(directory, &component, &directory);
        if (result != KERN_SUCCESS) {
            return result;
        }
    }

    return KERN_SUCCESS;
}

kern_return_t initrd_delete_file(struct initrd_filesystem *fs, const char *path) {
    if (!fs || !fs->root || !path) {
        return KERN_INVALID_ARGUMENT;
    }

    struct initrd_directory *parent;
    path_component_t leaf;
    kern_return_t result = find_parent_directory(fs, path, false, &parent, &leaf);
    if (result != KERN_SUCCESS) {
        return result;
    }

    for (size_t i = 0; i < parent->file_count; i++) {
        struct initrd_file *file = parent->files[i];
        if (strlen(file->name) != leaf.length ||
            !bytes_equal(file->name, leaf.start, leaf.length)) {
            continue;
        }

        kfree(file->data);
        kfree(file->name);
        kfree(file);
        if (i + 1 < parent->file_count) {
            memmove(&parent->files[i], &parent->files[i + 1],
                    (parent->file_count - i - 1) * sizeof(*parent->files));
        }
        parent->file_count--;
        return KERN_SUCCESS;
    }
    return KERN_NOT_FOUND;
}

kern_return_t initrd_read_file(struct initrd_filesystem *fs, const char *path, void **data,
                               size_t *size) {
    if (!fs || !fs->root || !path || !data || !size) {
        return KERN_INVALID_ARGUMENT;
    }

    struct initrd_directory *parent;
    path_component_t leaf;
    kern_return_t result = find_parent_directory(fs, path, false, &parent, &leaf);
    if (result != KERN_SUCCESS) {
        return result;
    }

    for (size_t i = 0; i < parent->file_count; i++) {
        struct initrd_file *file = parent->files[i];
        if (strlen(file->name) == leaf.length && bytes_equal(file->name, leaf.start, leaf.length)) {
            *data = file->data;
            *size = file->size;
            return KERN_SUCCESS;
        }
    }
    return KERN_NOT_FOUND;
}

static void initrd_print_tree_prefix(const struct initrd_tree_prefix *prefix) {
    if (prefix == NULL) {
        return;
    }
    initrd_print_tree_prefix(prefix->parent);
    kprintf("%s", prefix->is_last ? "    " : "|   ");
}

void initrd_print_tree(const initrd_directory_t *directory,
                       const struct initrd_tree_prefix *prefix) {
    for (size_t i = 0; i < directory->subdirectory_count; i++) {
        const initrd_directory_t *child = directory->subdirectories[i];
        bool is_last = i + 1 == directory->subdirectory_count && directory->file_count == 0;
        initrd_print_tree_prefix(prefix);
        kprintf("%s%s/\n", is_last ? "-- " : "|-- ", child->name);

        struct initrd_tree_prefix child_prefix = { .parent = prefix, .is_last = is_last };
        initrd_print_tree(child, &child_prefix);
    }

    for (size_t i = 0; i < directory->file_count; i++) {
        bool is_last = i + 1 == directory->file_count;
        initrd_print_tree_prefix(prefix);
        kprintf("%s%s\n", is_last ? "-- " : "|-- ", directory->files[i]->name);
    }
}
