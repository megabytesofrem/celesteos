#ifndef CELESTEOS_MACHO_H
#define CELESTEOS_MACHO_H

#include <cstdint>

namespace macho {

constexpr uint32_t MH_MAGIC_64 = 0xfeedfacf; /* The 64-bit Mach-O magic number */
constexpr uint32_t MH_CIGAM_64 = 0xcffaedfe; /* MH_MAGIC_64 but byte-swapped */

/* Load command types */
constexpr uint32_t LC_SEGMENT = 0x1;
constexpr uint32_t LC_SYMTAB = 0x2;
constexpr uint32_t LC_DYSYMTAB = 0xb;
constexpr uint32_t LC_SEGMENT_64 = 0x19;
constexpr uint32_t LC_LOAD_DYLINKER = 0xe;
constexpr uint32_t LC_FUNCTION_STARTS = 0x26;
constexpr uint32_t LC_DATA_IN_CODE = 0x29;
constexpr uint32_t LC_MAIN = 0x80000028;

struct __attribute__((packed)) mach_header_64 {
    uint32_t magic;
    uint32_t cputype;
    uint32_t cpusubtype;
    uint32_t filetype;
    uint32_t ncmds;
    uint32_t sizeofcmds;
    uint32_t flags;
    uint32_t reserved;
};

struct __attribute__((packed)) load_command {
    uint32_t cmd;
    uint32_t cmdsize;
};

struct __attribute__((packed)) segment_command_64 {
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
};

struct __attribute__((packed)) entry_point_command {
    uint32_t cmd;
    uint32_t cmdsize;
    uint64_t entryoff; /* offset of main() from start of __TEXT segment */
    uint64_t stacksize;
};

struct MachOInfo {
    uint64_t entrypoint;  /* entry point of the Mach-O binary */
    uint64_t text_vmaddr; /* virtual memory address of the __TEXT segment */
    uint64_t text_size;   /* size of the __TEXT segment */
    uint64_t data_vmaddr; /* virtual memory address of the __DATA segment */
    uint64_t data_size;   /* size of the __DATA segment */
};

const char* macho_describe_lc(uint32_t lc);
bool parse_macho(const uint8_t* data, MachOInfo& out_info);

}  // namespace macho

#endif  // CELESTEOS_MACHO_H