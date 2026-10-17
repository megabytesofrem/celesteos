#ifndef CELESTEOS_TAR_H
#define CELESTEOS_TAR_H

#include <stddef.h>
#include <stdint.h>

#include <mach/kern_return.h>

#define TAR_REGULAR_FILE  '0'
#define TAR_AREGULAR_FILE '\0'
#define TAR_HARD_LINK     '1'
#define TAR_SYM_LINK      '2'
#define TAR_DIRECTORY     '5'

#define TAR_BLOCK_SIZE 512

typedef struct __attribute__((packed)) tar_header
{
    char name[100];     /* file name */
    char mode[8];       /* permissions */
    char uid[8];        /* user ID */
    char gid[8];        /* group ID */
    char size[12];      /* file size */
    char mtime[12];     /* modification time */
    char chksum[8];     /* checksum */
    char typeflag;      /* type of file */
    char linkname[100]; /* name of linked file */
    char magic[6];      /* magic string */
    char version[2];    /* version */
    char uname[32];     /* user name */
    char gname[32];     /* group name */
    char devmajor[8];   /* major device number */
    char devminor[8];   /* minor device number */
    char prefix[155];   /* prefix for file name */
    char padding[12];   /* padding to make the header 512 bytes */
} tar_header_t;

typedef struct tar_archive
{
    void *base;
    size_t size;
} tar_archive_t;

typedef struct tar_file
{
    void *base;
    size_t size;
} tar_file_t;

kern_return_t
tar_lookup_file(const tar_archive_t *archive, const char *filename, tar_file_t *out_file);

#endif // CELESTEOS_TAR_H