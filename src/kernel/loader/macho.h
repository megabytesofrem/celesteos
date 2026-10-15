#ifndef CELESTEOS_MACHO_H
#define CELESTEOS_MACHO_H

#include <stdint.h>
#include <stdbool.h>

#define MH_MAGIC_64 0xfeedfacf /* The 64-bit Mach-O magic number */
#define MH_CIGAM_64 0xcffaedfe /* MH_MAGIC_64 but byte-swapped */

/* Load command types */
#define LC_SEGMENT 0x1
#define LC_SYMTAB 0x2
#define LC_DYSYMTAB 0xb
#define LC_SEGMENT_64 0x19
#define LC_LOAD_DYLINKER 0xe
#define LC_FUNCTION_STARTS 0x26
#define LC_DATA_IN_CODE 0x29
#define LC_MAIN 0x80000028

typedef struct __attribute__((packed)) mach_header_64 {
    uint32_t magic;
    uint32_t cputype;
    uint32_t cpusubtype;
    uint32_t filetype;
    uint32_t ncmds;
    uint32_t sizeofcmds;
    uint32_t flags;
    uint32_t reserved;
} mach_header_64;

typedef struct __attribute__((packed)) load_command {
    uint32_t cmd;
    uint32_t cmdsize;
} load_command;

typedef struct __attribute__((packed)) segment_command_64 {
    uint32_t cmd;
    uint32_t cmdsize;
    char segname[16];
    uint64_t vmaddr;
    uint64_t vmsize;
    uint64_t fileoff;
    uint64_t filesize;
    uint32_t maxprot;
    uint32_t initprot;
    uint32_t nsects;
    uint32_t flags;
} segment_command_64;

typedef struct __attribute__((packed)) entry_point_command {
    uint32_t cmd;
    uint32_t cmdsize;
    uint64_t entryoff; /* offset of main() from start of __TEXT segment */
    uint64_t stacksize;
} entry_point_command;

typedef struct __attribute__((packed)) macho_info {
    uint64_t entrypoint;  /* entry point of the Mach-O binary */
    uint64_t text_vmaddr; /* virtual memory address of the __TEXT segment */
    uint64_t text_size;   /* size of the __TEXT segment */
    uint64_t data_vmaddr; /* virtual memory address of the __DATA segment */
    uint64_t data_size;   /* size of the __DATA segment */
} macho_info;

const char* macho_describe_lc(uint32_t lc);
bool macho_parse(const uint8_t* data, struct macho_info* out_info);

#endif  // CELESTEOS_MACHO_H