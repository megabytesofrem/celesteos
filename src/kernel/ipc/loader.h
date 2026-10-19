#ifndef CELESTEOS_MACHO_H
#define CELESTEOS_MACHO_H

#include <stdbool.h>
#include <stdint.h>

#include <kernel/memory/pmap.h>
#include <mach/kern_return.h>

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

/* Entry point command (LC_MAIN) */
#define LC_MAIN 0x80000028

/**
 * A 64-bit Mach-O header.
 */
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

/**
 * A Mach-O load command.
 */
typedef struct __attribute__((packed)) load_command {
    uint32_t cmd;
    uint32_t cmdsize;
} load_command;

/**
 * A 64-bit Mach-O segment command.
 */
typedef struct __attribute__((packed)) segment_command_64 {
    uint32_t cmd;      /* LC_SEGMENT_64 */
    uint32_t cmdsize;  /* total size of this command */
    char segname[16];  /* segment name, e.g., "__TEXT" */
    uint64_t vmaddr;   /* virtual memory address of the segment */
    uint64_t vmsize;   /* memory size of the segment */
    uint64_t fileoff;  /* file offset of the segment */
    uint64_t filesize; /* size of the segment in the file */
    uint32_t maxprot;  /* maximum VM protection */
    uint32_t initprot; /* initial VM protection */
    uint32_t nsects;   /* number of sections in this segment */
    uint32_t flags;    /* segment flags */
} segment_command_64;

typedef struct __attribute__((packed)) symtab_command {
    uint32_t cmd;     /* LC_SYMTAB */
    uint32_t cmdsize; /* total size of this command */
    uint32_t symoff;  /* file offset of the symbol table */
    uint32_t nsyms;   /* number of symbol table entries */
    uint32_t stroff;  /* file offset of the string table */
    uint32_t strsize; /* size of the string table */
} symtab_command;

struct nlist_64 {
    uint32_t n_strx;  /* index into the string table */
    uint8_t n_type;   /* type of the symbol */
    uint8_t n_sect;   /* section number */
    uint16_t n_desc;  /* description field */
    uint64_t n_value; /* value of the symbol */
};

/**
 * A Mach-O entry point command (LC_MAIN).
 */
typedef struct __attribute__((packed)) entry_point_command {
    uint32_t cmd;       /* LC_MAIN */
    uint32_t cmdsize;   /* total size of this command */
    uint64_t entryoff;  /* offset of main() from start of __TEXT segment */
    uint64_t stacksize; /* initial stack size */
} entry_point_command;

/**
 * Contains information about a loaded Mach-O binary, including its entry point,
 * and addresses and offsets for the __TEXT and __DATA segments.
 */
typedef struct __attribute__((packed)) macho_info {
    uint64_t entrypoint;    /* entry point of the Mach-O binary */
    uint64_t stack_pointer; /* initial stack pointer for the Mach-O binary */

    /* Segment information */
    uint64_t text_vmaddr; /* virtual memory address of the __TEXT segment */
    uint64_t text_vmsize; /* size of the __TEXT segment */
    uint64_t data_vmaddr; /* virtual memory address of the __DATA segment */
    uint64_t data_vmsize; /* size of the __DATA segment */

    uint64_t text_fileoff;  /* file offset of the __TEXT segment */
    uint64_t text_filesize; /* size of the __TEXT segment */
    uint64_t data_fileoff;  /* file offset of the __DATA segment */
    uint64_t data_filesize; /* size of the __DATA segment */
} macho_info;

/**
 * Describe a Mach-O load command.
 */
const char *macho_describe_lc(uint32_t lc);

/**
 * Map a Mach-O segment into virtual memory, including its initial protections.
 */
kern_return_t map_segment(page_table_t *root, uint64_t vmaddr, uint64_t vmsize, uint32_t initprot,
                          uint64_t hhdm_base);

/**
 * Copy the payload of a Mach-O segment into mapped virtual memory,
 * starting from the specified segment file offset (`fileoff`).
 */
kern_return_t copy_segment_payload(page_table_t *root, uint64_t vmaddr, const uint8_t *payload,
                                   uint64_t fileoff, uint64_t filesz, uint64_t hhdm_base);

/**
 * Load a Mach-O binary into memory, mapping its segments and copying their payloads.
 *
 * This also allocates a user stack for the Mach-O binary to execute it in Ring 3.
 * `out_info` is populated with details about the loaded Mach-O binary.
 */
kern_return_t macho_load(const uint8_t *data, page_table_t *root, uint64_t hhdm_base,
                         struct macho_info *out_info);

#endif // CELESTEOS_MACHO_H