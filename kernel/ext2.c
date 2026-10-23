#include "ext2.h"

#include <errno.h>
#include <string.h>

#include <kernel/kmalloc.h>
#include <klog.h>

static int ext2_read_bytes(const ext2_fs_t *fs, uint64_t offset, void *buffer, size_t size) {
    if (fs == NULL || fs->image == NULL || (size != 0 && buffer == NULL)) {
        return -EINVAL;
    }

    if (offset > fs->image_size || size > fs->image_size - offset) {
        return -EINVAL;
    }

    if (size > 0) {
        memcpy(buffer, fs->image + offset, size);
    }
    return 0;
}

int ext2_mount_image(ext2_fs_t *fs, const uint8_t *image, size_t image_size) {
    if (!fs || !image || image_size == 0) {
        return -EINVAL;
    }

    // Ensure the file system is not already mounted; return -EBUSY if it is.
    if (fs->mounted) {
        return -EBUSY;
    }

    // Create the candidate filesystem; that is the filesystem which we validate before mounting
    ext2_fs_t candidate = { 0 };
    candidate.image = image;
    candidate.image_size = image_size;
    candidate.mounted = false;

    // Read its superblock from the image
    int result = ext2_read_superblock(&candidate, &candidate.superblock);
    if (result != 0) {
        return result;
    }
    const ext2_superblock_t *sb = &candidate.superblock;

    // Check the MAGIC number and validate basic superblock fields

    if (sb->magic != EXT2_SUPERBLOCK_MAGIC || sb->revision > EXT2_DYNAMIC_REV) {
        klog(KLOG_ERROR, "Bad superblock: magic=0x%x, revision=%u", sb->magic, sb->revision);
        return -EINVAL;
    }

    if (sb->inode_count < EXT2_ROOT_INODE) {
        klog(KLOG_ERROR, "Bad superblock: inode_count=%u", sb->inode_count);
        return -EINVAL;
    }

    if (sb->log_block_size > 2 || sb->blocks_per_group == 0 || sb->inodes_per_group == 0) {
        klog(KLOG_ERROR,
             "Bad superblock: log_block_size=%u, blocks_per_group=%u, inodes_per_group=%u",
             sb->log_block_size, sb->blocks_per_group, sb->inodes_per_group);
        return -EINVAL;
    }

    // Calculate the block size and inode size
    candidate.block_size = EXT2_MIN_BLOCK_SIZE << sb->log_block_size;
    candidate.inode_size =
        sb->revision == EXT2_GOOD_OLD_REV ? EXT2_GOOD_OLD_INODE_SIZE : sb->inode_size;

    // Validate the first data block and other superblock constraints.
    //
    // The first data block must be 1 for 1024-byte blocks and 0 for larger block sizes.
    if (sb->first_data_block != (candidate.block_size == 1024 ? 1u : 0u) ||
        sb->block_count <= sb->first_data_block || /* invalid block count */
        (uint64_t)sb->block_count * candidate.block_size > image_size ||
        sb->blocks_per_group > candidate.block_size * 8u || /* overflow check: blocks_per_group */
        sb->inodes_per_group > candidate.block_size * 8u || /* overflow check: inodes_per_group */
        candidate.inode_size < sizeof(ext2_inode_t) ||      /* inode size too small */
        candidate.inode_size > candidate.block_size ||      /* inode size too large */
        (candidate.inode_size & (candidate.inode_size - 1)) != 0 ||
        sb->log_fragment_size != (int32_t)sb->log_block_size ||
        sb->fragments_per_group != sb->blocks_per_group || sb->state != EXT2_VALID_FS) {

        klog(KLOG_ERROR,
             "Bad superblock: first_data_block=%u, block_count=%u, blocks_per_group=%u, "
             "inodes_per_group=%u, inode_size=%u, block_size=%u, log_fragment_size=%d, "
             "fragments_per_group=%u, state=%u",
             sb->first_data_block, sb->block_count, sb->blocks_per_group, sb->inodes_per_group,
             candidate.inode_size, candidate.block_size, sb->log_fragment_size,
             sb->fragments_per_group, sb->state);
        return -EINVAL;
    }

    // Calculate the number of data blocks and group descriptors.
    uint32_t data_blocks = sb->block_count - sb->first_data_block;
    candidate.group_count =
        data_blocks / sb->blocks_per_group + (data_blocks % sb->blocks_per_group != 0);

    // Calculate the offset and size of the group descriptor table within the image.
    uint64_t descriptor_offset = ((uint64_t)sb->first_data_block + 1) * candidate.block_size;
    uint64_t descriptor_size = (uint64_t)candidate.group_count * sizeof(ext2_group_descriptor_t);
    uint64_t filesystem_size = (uint64_t)sb->block_count * candidate.block_size;
    if (descriptor_offset > filesystem_size ||
        descriptor_size > filesystem_size - descriptor_offset ||
        sb->inode_count > (uint64_t)candidate.group_count * sb->inodes_per_group) {
        return -EINVAL;
    }

    // Allocate memory for the group descriptor table.
    ext2_group_descriptor_t *group_descriptors = kmalloc(descriptor_size);
    if (!group_descriptors) {
        return -ENOMEM;
    }

    // Read the group descriptor table from the filesystem image.
    if (ext2_read_bytes(&candidate, descriptor_offset, group_descriptors, descriptor_size) != 0) {
        kfree(group_descriptors);
        return -EINVAL;
    }
    candidate.group_descriptors = group_descriptors;

    // Assign the filesystem and mark as mounted
    candidate.mounted = true;
    *fs = candidate;
    return 0;
}

int ext2_unmount_image(ext2_fs_t *fs) {
    if (!fs) {
        return -EINVAL;
    }

    if (fs->group_descriptors) {
        kfree((void *)fs->group_descriptors);
        fs->group_descriptors = NULL;
    }

    fs->mounted = false;
    return 0;
}

bool ext2_is_mounted(const ext2_fs_t *fs) {
    if (!fs) {
        return false;
    }
    return fs->mounted;
}

int ext2_read_inode(const ext2_fs_t *fs, uint32_t inode_index, ext2_inode_t *inode) {
    if (!fs || !fs->image || !inode) {
        return -EINVAL;
    }

    if (inode_index == 0 || inode_index > fs->superblock.inode_count) {
        return -EINVAL;
    }

    uint32_t inodes_per_group = fs->superblock.inodes_per_group;
    uint32_t group_index = (inode_index - 1) / inodes_per_group;
    uint32_t local_index = (inode_index - 1) % inodes_per_group;

    if (group_index >= fs->group_count) {
        return -EINVAL;
    }

    const ext2_group_descriptor_t *group = &fs->group_descriptors[group_index];
    uint64_t inode_table_offset = (uint64_t)group->inode_table * fs->block_size;
    uint64_t inode_offset = inode_table_offset + (uint64_t)local_index * fs->inode_size;

    return ext2_read_bytes(fs, inode_offset, inode, sizeof(ext2_inode_t));
}

int ext2_read_block(const ext2_fs_t *fs, uint32_t block_index, void *buffer) {
    if (!fs || !fs->image || !buffer) {
        return -EINVAL;
    }

    // Calculate the byte offset of the block within the filesystem image.
    uint32_t block_size = fs->block_size;
    uint64_t offset = (uint64_t)block_index * block_size;

    // Read the block from the calculated offset.
    return ext2_read_bytes(fs, offset, buffer, block_size);
}

int ext2_read_superblock(const ext2_fs_t *fs, ext2_superblock_t *superblock) {
    if (!fs || !fs->image || !superblock) {
        return -EINVAL;
    }

    // The superblock is located at an offset of 1024 bytes from the start of the image.
    return ext2_read_bytes(fs, 1024, superblock, sizeof(ext2_superblock_t));
}

int ext2_read_file(const ext2_fs_t *fs, const ext2_inode_t *inode, void *buffer, size_t size) {
    if (!fs || !fs->image || !inode || !buffer) {
        return -EINVAL;
    }

    if (size == 0) {
        return 0;
    }

    if (!fs->mounted || fs->block_size == 0 || size > inode->size) {
        return -EINVAL;
    }
    if (size > (uint64_t)EXT2_DIRECT_BLOCK_COUNT * fs->block_size) {
        klog(KLOG_WARN, "ext2: indirect file reads are not implemented\n");
        return -ENOSYS;
    }

    uint32_t block_size = fs->block_size;
    size_t bytes_read = 0;
    for (unsigned int i = 0; bytes_read < size; i++) {
        size_t to_read = size - bytes_read > block_size ? block_size : size - bytes_read;
        if (inode->block[i] == 0) {
            memset((uint8_t *)buffer + bytes_read, 0, to_read);
            bytes_read += to_read;
            continue;
        }
        if (inode->block[i] >= fs->superblock.block_count) {
            return -EIO;
        }
        int ret = ext2_read_bytes(fs, (uint64_t)inode->block[i] * block_size,
                                  (uint8_t *)buffer + bytes_read, to_read);
        if (ret < 0) {
            return ret;
        }
        bytes_read += to_read;
    }

    return 0;
}

int ext2_list_directory(const ext2_fs_t *fs, const ext2_inode_t *inode, ext2_dir_entry_t *entries,
                        size_t entry_count) {

    if (!fs || !fs->image || !inode || !entries) {
        return -EINVAL;
    }

    (void)entry_count;
    klog(KLOG_WARN, "ext2: directory listing is not implemented\n");
    return -ENOSYS;
}

uint32_t ext2_lookup_path(const ext2_fs_t *fs, const char *path) {
    if (!fs || !fs->image || !path) {
        return 0;
    }

    uint32_t current = EXT2_ROOT_INODE;

    // Skip over any leading slashes
    while (*path == '/')
        path++;

    /**
     * When we reach the end of the path, we have found the inode corresponding to the path,
     * so we can just return it.
     */
    if (*path == '\0')
        return current;

    char path_component[256];

    while (*path != '\0') {
        bool found = false;

        // Read the next path component into the buffer, stopping at a slash
        // or a NULL terminator.
        size_t component_len = 0;
        while (*path != '/' && *path != '\0' && component_len < sizeof(path_component) - 1) {
            path_component[component_len++] = *path;
            path++;
        }
        path_component[component_len] = '\0';
        if (*path != '/' && *path != '\0') {
            return 0;
        }

        // Skip over any slashes after the component
        while (*path == '/')
            path++;

        /**
         * Our next step is to find the inode number of the directory entry matching
         * 'path_component' within the directory represented by 'current'. If it doesn't exist,
         * return 0.
         */
        ext2_inode_t dir_inode;
        if (ext2_read_inode(fs, current, &dir_inode) < 0) {
            return 0;
        }
        if ((dir_inode.mode & EXT2_INODE_TYPE_MASK) != EXT2_INODE_DIRECTORY) {
            return 0;
        }

        uint32_t dir_size = dir_inode.size;
        void *dir_buffer = kmalloc(dir_size);
        if (!dir_buffer) {
            return 0;
        }

        /**
         * Read the contents of the directory into the buffer.
         */
        if (ext2_read_file(fs, &dir_inode, dir_buffer, dir_size) < 0) {
            kfree(dir_buffer);
            return 0;
        }

        // Read each directory entry in the buffer and look for a match with 'path_component'.
        uint32_t dir_offset = 0;
        while (dir_offset < dir_size) {
            if (dir_size - dir_offset < sizeof(ext2_dir_entry_t)) {
                kfree(dir_buffer);
                return 0;
            }
            ext2_dir_entry_t *dir = (ext2_dir_entry_t *)((uint8_t *)dir_buffer + dir_offset);
            if (dir->record_len < sizeof(ext2_dir_entry_t) ||
                dir->record_len % 4 != 0 ||
                dir->record_len > dir_size - dir_offset ||
                dir->record_len > fs->block_size - dir_offset % fs->block_size ||
                dir->name_len > dir->record_len - sizeof(ext2_dir_entry_t)) {
                kfree(dir_buffer);
                return 0;
            }

            if (dir->inode != 0 && dir->name_len == component_len) {
                if (memcmp(dir->name, path_component, component_len) == 0) {
                    current = dir->inode;
                    found = true;
                    break;
                }
            }

            dir_offset += dir->record_len;
        }

        kfree(dir_buffer);

        if (!found)
            return 0;
    }

    return current;
}