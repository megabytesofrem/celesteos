#ifndef CELESTEOS_EXT2_H
#define CELESTEOS_EXT2_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* EXT2 filesystem constants */
#define EXT2_SUPERBLOCK_OFFSET 1024u
#define EXT2_SUPERBLOCK_SIZE 1024u
#define EXT2_SUPERBLOCK_MAGIC 0xEF53u
#define EXT2_ROOT_INODE 2u
#define EXT2_NAME_MAX 255u
#define EXT2_MIN_BLOCK_SIZE 1024u
#define EXT2_GOOD_OLD_REV 0u
#define EXT2_DYNAMIC_REV 1u
#define EXT2_GOOD_OLD_INODE_SIZE 128u
#define EXT2_GOOD_OLD_FIRST_INODE 11u

#define EXT2_VALID_FS 0x0001u
#define EXT2_ERROR_FS 0x0002u

/* Extended feature flags */
#define EXT2_FEATURE_COMPAT_DIR_PREALLOC 0x0001u
#define EXT2_FEATURE_COMPAT_HAS_JOURNAL 0x0004u
#define EXT2_FEATURE_COMPAT_EXT_ATTR 0x0008u
#define EXT2_FEATURE_COMPAT_RESIZE_INODE 0x0010u
#define EXT2_FEATURE_COMPAT_DIR_INDEX 0x0020u

#define EXT2_FEATURE_INCOMPAT_FILETYPE 0x0002u
#define EXT2_FEATURE_INCOMPAT_RECOVER 0x0004u
#define EXT2_FEATURE_INCOMPAT_JOURNAL_DEV 0x0008u
#define EXT2_FEATURE_INCOMPAT_META_BG 0x0010u

#define EXT2_FEATURE_RO_COMPAT_SPARSE_SUPER 0x0001u
#define EXT2_FEATURE_RO_COMPAT_LARGE_FILE 0x0002u
#define EXT2_FEATURE_RO_COMPAT_BTREE_DIR 0x0004u

/* File types */
#define EXT2_INODE_TYPE_MASK 0170000u
#define EXT2_INODE_FIFO 0010000u
#define EXT2_INODE_CHAR_DEVICE 0020000u
#define EXT2_INODE_DIRECTORY 0040000u
#define EXT2_INODE_BLOCK_DEVICE 0060000u
#define EXT2_INODE_REGULAR 0100000u
#define EXT2_INODE_SYMLINK 0120000u
#define EXT2_INODE_SOCKET 0140000u

#define EXT2_DIRECT_BLOCK_COUNT 12u
#define EXT2_INDIRECT_BLOCK_INDEX 12u
#define EXT2_DOUBLE_INDIRECT_BLOCK_INDEX 13u
#define EXT2_TRIPLE_INDIRECT_BLOCK_INDEX 14u
#define EXT2_INODE_BLOCK_COUNT 15u

typedef struct __attribute__((packed)) ext2_superblock {
    uint32_t inode_count;
    uint32_t block_count;
    uint32_t reserved_block_count;
    uint32_t free_block_count;
    uint32_t free_inode_count;
    uint32_t first_data_block;
    uint32_t log_block_size; /* Block size is 1024 << log_block_size. */
    int32_t log_fragment_size;
    uint32_t blocks_per_group;
    uint32_t fragments_per_group;
    uint32_t inodes_per_group;
    uint32_t mount_time;
    uint32_t write_time;
    uint16_t mount_count;
    int16_t max_mount_count;
    uint16_t magic;
    uint16_t state;
    uint16_t errors;
    uint16_t minor_revision;
    uint32_t last_check;
    uint32_t check_interval;
    uint32_t creator_os;
    uint32_t revision;
    uint16_t reserved_uid;
    uint16_t reserved_gid;

    /* Revision 1 fields; revision 0 uses fixed inode size and first-inode values. */
    uint32_t first_inode;
    uint16_t inode_size;
    uint16_t block_group_number;
    uint32_t feature_compat;
    uint32_t feature_incompat;
    uint32_t feature_ro_compat;
    uint8_t uuid[16];
    char volume_name[16];
    char last_mounted[64];
    uint32_t algorithm_usage_bitmap;
    uint8_t extended_fields[820];
} ext2_superblock_t;

typedef struct __attribute__((packed)) ext2_group_descriptor {
    uint32_t block_bitmap;
    uint32_t inode_bitmap;
    uint32_t inode_table;
    uint16_t free_block_count;
    uint16_t free_inode_count;
    uint16_t used_directory_count;
    uint16_t padding;
    uint32_t reserved[3];
} ext2_group_descriptor_t;

typedef struct __attribute__((packed)) ext2_inode {
    uint16_t mode;
    uint16_t uid;
    uint32_t size;
    uint32_t access_time;
    uint32_t change_time;
    uint32_t modification_time;
    uint32_t deletion_time;
    uint16_t gid;
    uint16_t link_count;
    uint32_t sector_count; /* Allocated 512-byte sectors, not filesystem blocks. */
    uint32_t flags;
    uint32_t os_specific_1;
    uint32_t block[EXT2_INODE_BLOCK_COUNT];
    uint32_t generation;
    uint32_t file_acl;
    uint32_t directory_acl; /* Directory ACL or high 32 bits of file size. */
    uint32_t fragment_address;
    uint8_t os_specific_2[12];
} ext2_inode_t;

typedef enum ext2_directory_file_type {
    EXT2_FT_UNKNOWN = 0,
    EXT2_FT_REGULAR = 1,
    EXT2_FT_DIRECTORY = 2,
    EXT2_FT_CHAR_DEVICE = 3,
    EXT2_FT_BLOCK_DEVICE = 4,
    EXT2_FT_FIFO = 5,
    EXT2_FT_SOCKET = 6,
    EXT2_FT_SYMLINK = 7
} ext2_directory_file_type_t;

typedef struct __attribute__((packed)) ext2_dir_entry {
    uint32_t inode;
    uint16_t record_len; /* Includes padding; records are aligned to four bytes. */
    uint8_t name_len;
    uint8_t file_type;
    char name[]; /* Not NUL-terminated. */
} ext2_dir_entry_t;

/*
 * In memory ext2 filesystem.
 */
typedef struct ext2_fs {
    const uint8_t *image; /* Starts at the filesystem, not a disk partition table. */
    size_t image_size;

    ext2_superblock_t superblock;
    const ext2_group_descriptor_t *group_descriptors;
    uint32_t group_count;

    uint32_t block_size;
    uint16_t inode_size;     /* On-disk stride; may exceed sizeof(ext2_inode_t). */
    ext2_inode_t root_inode; /* Cached base record for EXT2_ROOT_INODE. */

    bool mounted;
} ext2_fs_t;

typedef struct vfs {
    const char *mount_point;
    ext2_fs_t *fs;
} vfs_t;

/* Borrows the image; currently validates baseline geometry, not all inode metadata. */
int ext2_mount_image(ext2_fs_t *fs, const uint8_t *image, size_t image_size);
int ext2_unmount_image(ext2_fs_t *fs);

/* Check whether the filesystem is mounted. */
bool ext2_is_mounted(const ext2_fs_t *fs);

/**
 * Read an inode from the filesystem by its 1-based index.
 */
int ext2_read_inode(const ext2_fs_t *fs, uint32_t inode_index, ext2_inode_t *inode);

/**
 * Read a block from the filesystem.
 */
int ext2_read_block(const ext2_fs_t *fs, uint32_t block_index, void *buffer);

/**
 * Read the superblock from the filesystem.
 */
int ext2_read_superblock(const ext2_fs_t *fs, ext2_superblock_t *superblock);

/**
 * Read the contents of a file (represented by an inode internally).
 */
int ext2_read_file(const ext2_fs_t *fs, const ext2_inode_t *inode, void *buffer, size_t size);

/**
 * List the contents of a directory (represented by an inode internally).
 */
int ext2_list_directory(const ext2_fs_t *fs, const ext2_inode_t *inode, ext2_dir_entry_t *entries,
                        size_t entry_count);

uint32_t ext2_lookup_path(const ext2_fs_t *fs, const char *path);

#endif /* CELESTEOS_EXT2_H */