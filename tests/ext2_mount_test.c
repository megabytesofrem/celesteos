#include <kernel/ext2.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    assert(argc == 2);
    FILE *file = fopen(argv[1], "rb");
    assert(file != NULL);
    assert(fseek(file, 0, SEEK_END) == 0);
    long length = ftell(file);
    assert(length > 0);
    rewind(file);
    uint8_t *image = malloc((size_t)length);
    assert(image != NULL);
    assert(fread(image, 1, (size_t)length, file) == (size_t)length);
    assert(fclose(file) == 0);

    ext2_fs_t fs = ext2_create_fs(image, (size_t)length);
    assert(!fs.mounted && fs.block_size == 0 && fs.group_descriptors == NULL);
    assert(ext2_mount_image(&fs, image, (size_t)length) == 0);
    assert(fs.mounted && fs.block_size == 1024 && fs.inode_size == 128);
    assert(fs.group_count != 0 && fs.group_descriptors != NULL);
    assert(ext2_mount_image(&fs, image, (size_t)length) < 0);
    assert(fs.mounted);
    assert(ext2_unmount_image(&fs) == 0);
    assert(!fs.mounted && fs.image == NULL && fs.group_descriptors == NULL);

    assert(ext2_mount_image(&fs, image, EXT2_SUPERBLOCK_OFFSET) < 0);
    assert(!fs.mounted);
    ext2_superblock_t *sb = (ext2_superblock_t *)(image + EXT2_SUPERBLOCK_OFFSET);
    uint16_t magic = sb->magic;
    sb->magic = 0;
    assert(ext2_mount_image(&fs, image, (size_t)length) < 0);
    assert(!fs.mounted);
    sb->magic = magic;
    uint32_t shift = sb->log_block_size;
    sb->log_block_size = 32;
    assert(ext2_mount_image(&fs, image, (size_t)length) < 0);
    assert(!fs.mounted);
    sb->log_block_size = shift;
    sb->feature_incompat |= EXT2_FEATURE_INCOMPAT_META_BG;
    assert(ext2_mount_image(&fs, image, (size_t)length) < 0);
    assert(!fs.mounted);

    free(image);
    puts("ext2 mount tests passed");
    return 0;
}
