# Next steps: a read-only ext2 reader

## Goal and scope

The first useful milestone is:

> Given a memory-resident ext2 image, validate its metadata, list its root
> directory, resolve a pathname, and read a small regular file.

Develop this as an image parser first. Disk hardware, a generic VFS, userspace
file descriptors, and writable filesystems are separate milestones.

Initial scope:

- An image beginning at the filesystem's byte zero, with no partition table.
- Read-only access, with the caller retaining the backing image.
- Classic ext2 revisions 0 and 1.
- Block sizes of 1,024, 2,048, and 4,096 bytes.
- Base inode records with a validated on-disk stride.
- Linear directories.
- Direct data blocks first, then single-indirect blocks.
- Explicit errors for unsupported features and operations.

Use filesystem format documentation to write original implementation code.
Follow [AGENTS.md](./AGENTS.md); no XNU implementation code is needed.

## Current repository state

- [kernel/ext2.h](./kernel/ext2.h) defines the disk structures and `ext2_fs_t`.
- [kernel/ext2.c](./kernel/ext2.c) contains preliminary constructors and stubs.
- `ext2_create_fs` zero-initializes its context. `ext2_mount_image` now checks
  baseline superblock fields, feature flags, geometry, and descriptor-table bounds.
- Full descriptor/inode validation, root-inode loading, pathname lookup, and
  correct general-purpose file reads remain to be implemented.
- Several declared functions, including unmount and file/block reads, are not
  implemented yet.
- Ext2 is linked into the kernel. `make initrd` creates `build/initrd.ext2`;
  `make iso` stages it as a Limine module identified by `ext2-initrd`.
- The kernel retains that module in a static filesystem context, logs the
  geometry, resolves `/usr/libexec/launchd`, loads its Mach-O image, and enters
  user mode. This boot path is limited to direct-block files and directories;
  general-purpose ext2 support remains unfinished.
- `make test-ext2` exercises baseline mounting and several invalid-image cases.
- There is no current VFS implementation to integrate with.

Do not interpret successful initialization as a complete working filesystem.
Unimplemented functions should return an explicit error, not leave callers with
uninitialized outputs.

## Suggested implementation order

| Stage | Deliverable | Completion check |
|---|---|---|
| 1 | Safe image reads and initialized context | Truncated and overflowing ranges fail |
| 2 | Validated superblock and geometry | All three supported block sizes mount |
| 3 | Group descriptors and inode reads | Root inode matches `debugfs` |
| 4 | Direct-block file reads | Small file contents match byte-for-byte |
| 5 | Directory iteration and pathname lookup | Nested files resolve correctly |
| 6 | Single-indirect blocks and holes | Large and sparse files read correctly |
| 7 | Kernel boot-image integration | Kernel reads a known file from a module |
| Later | VFS, disk backend, writes | Separate designs and tests |

## 1. Establish initialization and ownership rules

Fix the constructor first:

```c
ext2_fs_t ext2_create_fs(const uint8_t *image, size_t image_size) {
    ext2_fs_t fs = {0};
    fs.image = image;
    fs.image_size = image_size;
    return fs;
}
```

This initializes the context; it does not validate or mount it.

Define these rules before implementing the remaining functions:

- The caller owns the image and keeps it unchanged and alive while mounted.
- The filesystem context borrows the image; unmount does not free it.
- Group-descriptor pointers, if retained, point into that same image.
- The superblock and root inode are cached copies.
- Derived values such as `block_size` and `group_count` are host-endian.
- Disk fields are little-endian. On current x86_64 targets this matches native
  byte order, but do not silently assume that for future architectures.
- Concurrent access and mutation are outside the initial implementation's scope.

Avoid packing `ext2_fs_t`: it is runtime state, not an on-disk format.

### Mount API

The existing API accepts a device name and mount point, but there is no device
lookup or mount namespace behind those strings. Do not imply that it opens a
device or registers a mount.

A suitable eventual parser API is:

```c
int ext2_mount_image(ext2_fs_t *fs, const uint8_t *image, size_t image_size);
void ext2_unmount_image(ext2_fs_t *fs);
```

These are proposed interfaces, not implemented functions. Either introduce them
or clearly document the limitations of the existing interface before using it.
Let a future VFS own pathname-to-mount resolution.

## 2. Implement one bounded byte-copy helper

All metadata and file reads should ultimately use the same range-checking rule.
For example, as a private helper in `ext2.c`:

```c
static int ext2_read_bytes(const ext2_fs_t *fs, uint64_t offset,
                           void *buffer, size_t size) {
    if (fs == NULL || fs->image == NULL || (size != 0 && buffer == NULL)) {
        return -EINVAL;
    }
    if (offset > fs->image_size || size > fs->image_size - offset) {
        return -EIO;
    }
    if (size != 0) {
        memcpy(buffer, fs->image + (size_t)offset, size);
    }
    return 0;
}
```

Include the appropriate headers when implementing this example.
It intentionally does not require `mounted`: mounting needs to read metadata
before the context can be marked mounted.

Important details:

- Check the range before pointer arithmetic.
- Avoid `offset + size > image_size`, because addition can overflow.
- A zero-byte read at the image end is valid under this helper's contract.
- Use `uint64_t` for intermediate block and inode offset arithmetic.
- Validation against the image is not enough by itself: also validate against
  the filesystem's declared block count.

For a block number:

```c
uint64_t offset = (uint64_t)block_number * fs->block_size;
```

Widen before multiplying, not after.

## 3. Read the superblock and validate geometry

Read exactly 1,024 bytes at image offset 1,024. This position does not change
with the filesystem block size.

Restore compile-time layout checks in the header, including:

```c
_Static_assert(sizeof(ext2_superblock_t) == 1024, "bad superblock layout");
_Static_assert(offsetof(ext2_superblock_t, magic) == 56, "bad magic offset");
_Static_assert(sizeof(ext2_group_descriptor_t) == 32, "bad group descriptor");
_Static_assert(sizeof(ext2_inode_t) == 128, "bad base inode layout");
_Static_assert(offsetof(ext2_inode_t, block) == 40, "bad block pointer offset");
_Static_assert(sizeof(ext2_directory_entry_t) == 8, "bad directory header");
```

Validate before publishing a mounted context:

1. The complete superblock fits in the image.
2. Magic equals `EXT2_SUPERBLOCK_MAGIC`.
3. Revision is 0 or 1.
4. `log_block_size <= 2` for the initial 1/2/4 KiB implementation.
5. Only after that check, compute `1024u << log_block_size`.
6. `first_data_block` is 1 for 1 KiB blocks, otherwise 0.
7. `block_count > first_data_block`, and inode count includes inode 2.
8. Blocks and inodes per group are nonzero.
9. Each per-group count fits its one-block bitmap: at most `block_size * 8`.
10. `(uint64_t)block_count * block_size` fits within the image.
11. Revision 0 uses inode size 128 and first nonreserved inode 11; do not trust
    revision-1-only fields on a revision-0 image.
12. Revision 1's inode stride is a supported power of two, at least 128, and no
    larger than the block size.
13. Reject unsupported fragment geometry initially.
14. Reject images requiring recovery or using layouts you do not implement.

### Choose an explicit feature policy

A deliberately conservative initial policy is:

- No compatible feature bits accepted initially.
- Only `EXT2_FEATURE_INCOMPAT_FILETYPE` accepted among incompatible bits.
- No read-only-compatible feature bits accepted initially.
- Require a clean filesystem state; do not attempt recovery.

This is stricter than a general ext2 reader needs to be, but makes the first
implementation easier to reason about. In particular, reject journaled images,
`META_BG`, extents, and 64-bit descriptor layouts. Their metadata must not be
interpreted as classic ext2 structures.

After baseline tests pass, add support deliberately:

- `SPARSE_SUPER` does not change where the primary descriptor table lives.
- `LARGE_FILE` requires proper 64-bit regular-file size handling.
- Some compatible features can be safely ignored for read-only access, but
  document why before accepting them.

Unknown incompatible bits always need rejection. Do not infer support from
the image's `.img` name or the tool's `-t ext2` option.

### Publish mount state last

Build a temporary context, validate descriptors and root inode, then commit it:

```text
zero temporary context
attach borrowed image
read and validate superblock
derive and validate geometry
validate descriptor table
read and validate root inode
set mounted = true
assign completed context to caller
```

Use internal metadata-reading helpers that can operate during mount.
Public operations should reject unmounted contexts.

Do not overwrite an already mounted context without an explicit remount policy.
On failure, leave the destination unchanged or explicitly unmounted according
to a documented contract; never leave partially initialized state marked mounted.

## 4. Locate and validate group descriptors

For the classic layout, the primary descriptor table begins at:

```text
descriptor_offset = (first_data_block + 1) * block_size
```

Use 64-bit arithmetic:

```c
uint64_t descriptor_offset =
    ((uint64_t)superblock.first_data_block + 1) * block_size;
```

Examples:

| Block size | First data block | Descriptor table offset |
|---|---|---|
| 1,024 | 1 | 2,048 |
| 2,048 | 0 | 2,048 |
| 4,096 | 0 | 4,096 |

Compute the group count without an overflowing round-up addition:

```text
data_blocks = block_count - first_data_block
group_count = data_blocks / blocks_per_group
            + (data_blocks % blocks_per_group != 0)
```

Check that inode count and per-group inode geometry are consistent with that
count. Validate the entire descriptor table against both image bounds and the
declared filesystem size.

For each descriptor, validate the inode-table span, not just its first block.
Also validate bitmap block locations. Respect the last group's potentially
smaller size. A strict initial reader can require ordinary, group-local metadata
because alternative placement features are not supported.

You do not need allocation or bitmap updates to read files. Those are part of
the later writable-filesystem work.

## 5. Implement inode reads

For a 1-based inode number `n`:

```text
group = (n - 1) / inodes_per_group
index = (n - 1) % inodes_per_group

inode_offset = group[group].inode_table * block_size
             + index * inode_size
```

Before subtracting 1, require `1 <= n <= inode_count`.
Validate the group index and the resulting range.

Copy only the 128-byte base record into `ext2_inode_t`; advance between records
using the validated inode stride, which may be 256 bytes or another supported
size.

Root inode 2 must be a directory:

```c
if ((root.mode & EXT2_INODE_TYPE_MASK) != EXT2_INODE_DIRECTORY) {
    return -EINVAL;
}
```

Keep these distinctions clear:

- `sector_count` counts allocated 512-byte sectors, not filesystem blocks.
- File length comes from the size fields, not `sector_count`.
- The field currently named `directory_acl` can supply the upper 32 bits of
  regular-file size when the applicable revision/feature rules permit it.
- Do not apply that high-size interpretation indiscriminately to directories
  or other inode types.
- Symlinks and device nodes cannot be treated as regular data-block files.

## 6. Read direct-block regular files

Add a logical-to-physical block helper:

```c
static int ext2_lookup_data_block(const ext2_fs_t *fs,
                                  const ext2_inode_t *inode,
                                  uint64_t logical_block,
                                  uint32_t *physical_block);
```

Initially:

- Logical blocks 0 through 11 use `inode->block[logical_block]`.
- Zero pointers represent holes; the caller fills those bytes with zeroes.
- Every nonzero pointer must be within the declared filesystem block range.
- Logical blocks requiring unimplemented indirection return an explicit error.

Never return success with truncated data simply because the reader has not
implemented the next level of indirection.

A useful file-reading API is:

```c
int ext2_pread(const ext2_fs_t *fs, const ext2_inode_t *inode,
               uint64_t offset, void *buffer, size_t capacity,
               size_t *bytes_read);
```

Suggested contract:

- Return 0 for success, including EOF.
- Return a negative error on failure.
- Initialize `*bytes_read = 0` before reading.
- On an error after progress, retain the number of bytes actually copied.
- Do not read beyond the inode's file length.
- Reject inode types not supported by this operation.

For each iteration:

```text
logical_block = offset / block_size
within_block = offset % block_size
chunk = min(block_size - within_block,
            remaining_buffer_capacity,
            file_size - offset)
```

Resolve the physical block, read the chunk or zero-fill a hole, and advance.
Check `offset >= file_size` before subtracting `file_size - offset`.

At 1 KiB blocks the direct pointers cover 12 KiB. At 4 KiB they cover 48 KiB.
These limits make useful boundary tests.

## 7. Iterate directories safely

A directory is a file containing variable-length records, not an array of
`ext2_directory_entry_t`.

For each record:

1. Ensure at least the 8-byte header remains in the current block and directory.
2. Copy/decode the header without assuming naturally aligned disk data.
3. Require `record_length >= 8`.
4. Require `record_length % 4 == 0`.
5. Require the record to fit in the current filesystem block and directory size.
6. Decode the name length according to the `FILETYPE` feature flag.
7. Require the name length to be at most 255 and fit within `record_length - 8`.
8. An inode number of zero marks an unused record: skip it, but still validate
   the record length so iteration cannot loop forever.
9. Nonzero inode numbers must be in range.
10. Advance by `record_length`, not by header size plus name length.

Without `EXT2_FEATURE_INCOMPAT_FILETYPE`, bytes 6 and 7 form a little-endian
16-bit name length. With it, byte 6 is the name length and byte 7 is the file type.
Do not require a valid file-type byte on old-format directories.

Names are not NUL-terminated:

```c
if (disk_name_length == wanted_length &&
    memcmp(disk_name, wanted, wanted_length) == 0) {
    /* Found the entry. */
}
```

For display, copy a validated name into `char name[EXT2_NAME_MAX + 1]` and append
a terminator. Do not pass the raw directory name to `%s`.

Start by listing inode 2. Then implement lookup of one component within a
directory, followed by multi-component pathname lookup.

Example:

```text
/usr/libexec/launchd
  inode 2 -> lookup "usr"
          -> lookup "libexec"
          -> lookup "launchd"
```

Read each intermediate inode and verify it is a directory. Define behavior for
repeated slashes, trailing slashes, `.` and `..`. Reject symlink traversal
explicitly until it is implemented; later add a traversal limit.

## 8. Add single-indirect blocks

An indirect block contains `block_size / 4` little-endian block numbers.

Let `p = block_size / 4`:

```text
logical blocks 0 .. 11       -> direct pointers
logical blocks 12 .. 12+p-1  -> single-indirect pointer block
```

For a single-indirect lookup:

```text
entry_index = logical_block - 12
pointer_block = inode.block[12]
pointer_offset = pointer_block * block_size + entry_index * 4
```

If the pointer block itself is zero, the covered region is a hole. Otherwise
validate it before reading the requested pointer; validate the resulting data
block too.

Double and triple indirection can follow later. Use the same checked primitive
at every level, and use wide arithmetic for the coverage calculations.

## 9. Generate disposable test images

Host tools: `mke2fs`, `debugfs`, `dumpe2fs`, and `e2fsck` from e2fsprogs.
The examples also use `truncate`, `dd`, and standard file utilities.

These commands create or overwrite only the named test artifacts. Do not point
them at a real disk device or an image you want to preserve.

### Small baseline image

From the repository root:

```sh
mkdir -p build/ext2-tests/input/docs
printf 'hello from ext2\n' > build/ext2-tests/input/hello.txt
printf 'nested file\n' > build/ext2-tests/input/docs/nested.txt
dd if=/dev/zero of=build/ext2-tests/input/large.bin bs=1024 count=20 status=none

truncate -s 16M build/ext2-tests/baseline.img
mke2fs -t ext2 -F -b 1024 -I 128 -O none \
    -d build/ext2-tests/input build/ext2-tests/baseline.img
```

The 20 KiB file crosses the direct-block limit at 1 KiB blocks. It should produce
an explicit unsupported error before single-indirect support exists, then read
correctly afterward. The zero-filled contents may also exercise sparse-block
handling depending on how the image builder stores the file.

For a guaranteed nonzero-content indirect test, create another fixture with
varying bytes in the host test program and compare it byte-for-byte after reading.

Some tool versions warn about 128-byte inodes; they are intentional for this
baseline. Test 256-byte inode strides separately rather than assuming all ext2
images use 128 bytes.

### Inspect expected results

```sh
dumpe2fs -h build/ext2-tests/baseline.img
debugfs -R 'stat /' build/ext2-tests/baseline.img
debugfs -R 'ls -l /' build/ext2-tests/baseline.img
debugfs -R 'stat /hello.txt' build/ext2-tests/baseline.img
debugfs -R 'cat /hello.txt' build/ext2-tests/baseline.img
e2fsck -fn build/ext2-tests/baseline.img
```

Use the displayed geometry, inode numbers, and block mappings as independent
expected values for the host tests.

Also generate separate images with:

- `-b 2048` and `-b 4096`.
- `-I 256`.
- `-O filetype` for directory type fields.
- `-r 0 -O none -I 128` for an explicit revision-0 fixture.

Inspect each image's actual features with `dumpe2fs`; do not rely on system-wide
`mke2fs` defaults. Add features only after the corresponding reader support.

## 10. Add host-side tests before kernel integration

Expand `tests/ext2_mount_test.c` and the existing `make test-ext2` target, or add
separate parser/file-reading tests as those operations become available.

The harness can use host `fopen`/`fread` to load the complete image into a buffer,
then pass that buffer to the same parser used in the kernel.

Avoid pulling framebuffer, scheduler, or privileged CPU code into these tests.
Use explicit include paths so host libc headers do not accidentally resolve to
the kernel's minimal `string.h`. Audit the selected errno definitions as well:
error numbers and host-library errno behavior are separate from parser logic.

Once the test exists, a useful build shape is:

```sh
cc -std=c17 -Wall -Wextra -Werror -g \
    -fsanitize=address,undefined \
    -I. tests/ext2_test.c kernel/ext2.c \
    -o build/ext2-tests/ext2_test

build/ext2-tests/ext2_test build/ext2-tests/baseline.img
```

Adapt the include paths and command-line interface to the actual test harness.
Sanitizers are host-only validation tools, not kernel build flags.

### Required tests

Valid images:

- [ ] Root directory and a known regular file.
- [ ] Nested pathname resolution.
- [ ] All three supported block sizes.
- [ ] 128-byte and 256-byte inode strides.
- [ ] Both directory-entry name-length formats.
- [ ] Empty file and zero-length read.
- [ ] Exact EOF, read past EOF, and reads crossing block boundaries.
- [ ] Last direct block and first single-indirect block.
- [ ] Sparse holes return zeroes without reading block zero.
- [ ] A non-NUL-terminated maximum-length directory name.

Malformed or unsupported images:

- [ ] Missing or truncated superblock.
- [ ] Bad magic or unsupported revision.
- [ ] Oversized block shift, zero group sizes, invalid inode stride.
- [ ] Declared filesystem larger than the backing image.
- [ ] Truncated descriptor table or inode-table span.
- [ ] Inode number zero or beyond inode count.
- [ ] Invalid data and indirect block numbers.
- [ ] Zero, undersized, misaligned, or overlong directory records.
- [ ] Name length exceeding its record or the filesystem name limit.
- [ ] Unknown incompatible features and recovery-required images.
- [ ] Unsupported symlinks, indexed-directory layouts, or indirection levels.
- [ ] Failed mount never leaves `mounted == true`.

Mutate copies in memory for failure tests rather than damaging the good fixture.
Check that failed reads do not overwrite bytes outside the requested output
range and that reported byte counts follow the documented contract.

## 11. Extend the initial kernel integration after host tests pass

The image-build/module/context plumbing below is now wired. Continue with
validated inode reads and pathname lookup before attempting userspace loading.

1. Keep the module memory mapped and alive while `ext2_fs_t` references it.
2. Extend mount-time validation to descriptors, inode tables, and the root inode.
3. List the root directory and read a known test file.
4. Resolve `/usr/libexec/launchd` and read its complete contents.
5. Pass those bytes to the Mach-O loader and restore the usermode transition.

The previous tar path is removed/commented out in the current tree. Do not
claim that userspace boot has been restored until ext2 can reliably locate and
read the binary. Image initialization alone is not a sufficient boot-path test.

## Later work, deliberately deferred

- A filesystem-neutral VFS mount object with operations and private context.
- Mapping BSD `open`, `read`, and `close` onto file objects and offsets.
- A block-device backend replacing or supplementing memory-image reads.
- Double/triple indirection, symlinks, and broader feature support.
- Writable ext2: allocation bitmaps, metadata updates, directory changes,
  ordering, and crash consistency.

A readable ext2 image is a manageable first project. A writable filesystem is
substantially more work; do not combine the two milestones.

## References

- [The Second Extended File System documentation](https://www.nongnu.org/ext2-doc/ext2.html)
  for the disk format and field meanings.
- [OSDev Ext2 overview](https://wiki.osdev.org/Ext2) for an implementation-oriented
  introduction.
- Local manuals: `man mke2fs`, `man debugfs`, `man dumpe2fs`, and `man e2fsck`.

The examples in this document are implementation guidance, not a completed or
validated ext2 driver. Keep the checked-in header and implementation synchronized
as each stage is completed.
