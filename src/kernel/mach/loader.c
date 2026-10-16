#include "loader.h"

#include <stdbool.h>

#include <stdlib/string.h>
#include <stdlib/libkernel.h>

#include <kernel/memory/pmm.h>
#include <kernel/memory/vmm.h>

/**
 * __TEXT is readable (PAGE_PRESENT) and executable (PAGE_EXECUTE), but not writable.
 * __DATA is readable (PAGE_PRESENT) and writable (PAGE_READ_WRITE), but not executable.
 * 
 * __LINKEDIT contains data used by the linker, typically read-only. Its contents are described by
 * load commands e.g LC_SYMTAB, LC_DYSYMTAB, etc.
 */

const char* macho_describe_lc(uint32_t lc) {
    switch (lc) {
        case LC_SEGMENT:
            return "LC_SEGMENT";
        case LC_SYMTAB:
            return "LC_SYMTAB";
        case LC_DYSYMTAB:
            return "LC_DYSYMTAB";
        case LC_SEGMENT_64:
            return "LC_SEGMENT_64";
        case LC_LOAD_DYLINKER:
            return "LC_LOAD_DYLINKER";
        case LC_FUNCTION_STARTS:
            return "LC_FUNCTION_STARTS";
        case LC_DATA_IN_CODE:
            return "LC_DATA_IN_CODE";
        case LC_MAIN:
            return "LC_MAIN";
        default:
            return "UNKNOWN";
    }
}

static inline size_t page_align_up(size_t size) {
    return (size + VMM_PAGE_SIZE - 1) & ~((size_t)VMM_PAGE_SIZE - 1);
}

static inline size_t page_align_down(uintptr_t addr) {
    return addr & ~((uintptr_t)VMM_PAGE_SIZE - 1);
}

bool map_segment(page_table_t* root, uint64_t vmaddr, uint64_t vmsize, uint64_t hhdm_base) {
    if (vmsize == 0) return true;

    uintptr_t vm_start = page_align_down(vmaddr);
    size_t offset = vmaddr - vm_start;
    if (vmsize > SIZE_MAX - offset - (VMM_PAGE_SIZE - 1) ||
        vmaddr > UINT64_MAX - vmsize) {
        klog(KLOG_ERROR, "macho-loader: Segment address range overflows.\n");
        return false;
    }
    size_t page_count = page_align_up(offset + vmsize) / VMM_PAGE_SIZE;

    // Allocate backing physical pages
    void* phys_pages = pmm_alloc_pages(page_count);
    if (!phys_pages) {
        klog(KLOG_ERROR, "macho-loader: Failed physical allocation for segment at 0x%p\n", vmaddr);
        return false;
    }

    uintptr_t phys_addr = (uintptr_t)phys_pages;

    // Zero out memory via HHDM (automatically zeroes uninitialized BSS space)
    memset((void*)(phys_addr + hhdm_base), 0, page_count * VMM_PAGE_SIZE);

    // Map physical frames into user virtual memory space
    for (size_t i = 0; i < page_count; ++i) {
        if (!vmm_map(root,
                     vm_start + (i * VMM_PAGE_SIZE),
                     phys_addr + (i * VMM_PAGE_SIZE),
                     PAGE_PRESENT | PAGE_USER | PAGE_READ_WRITE)) {
            klog(KLOG_ERROR, "macho-loader: Failed to map page %x for segment at 0x%p\n", i, vmaddr);
            return false;
        }
    }
    return true;
}

bool copy_segment_payload(page_table_t* root, uint64_t vmaddr, const uint8_t* payload, uint64_t fileoff, uint64_t filesz, uint64_t hhdm_base) {
    if (filesz == 0) return true;

    size_t copied = 0;
    while (copied < filesz) {
        uintptr_t curr_user_addr = vmaddr + copied;
        uintptr_t phys_addr = vmm_get_physical(root, curr_user_addr);

        if (!phys_addr) {
            klog(KLOG_ERROR, "macho-loader: Failed to resolve physical frame for 0x%p\n", curr_user_addr);
            return false;
        }

        size_t page_offset = curr_user_addr & (VMM_PAGE_SIZE - 1);
        size_t chunk_size = VMM_PAGE_SIZE - page_offset;
        if (chunk_size > (filesz - copied)) {
            chunk_size = filesz - copied;
        }

        // Copy directly into physical frame using higher-half direct map
        memcpy((void*)(phys_addr + hhdm_base + page_offset), payload + fileoff + copied, chunk_size);
        copied += chunk_size;
    }
    return true;
}

bool macho_parse(const uint8_t* data, struct macho_info* out_info) {
    *out_info = (struct macho_info){0};
    const mach_header_64* header = (const mach_header_64*)(data);

    // Validate the Mach-O header magic numbers
    if (header->magic != MH_MAGIC_64 && header->magic != MH_CIGAM_64) {
        klog(KLOG_ERROR, "macho-loader: Invalid Mach-O magic number: %x\n", header->magic);
        return false;
    }

    // Advance to the first load command
    const uint8_t* load_cmd_ptr = data + sizeof(mach_header_64);

    // Read each load command
    for (uint32_t i = 0; i < header->ncmds; ++i) {
        const load_command* lc = (const load_command*)(load_cmd_ptr);

        /**
         * Stubs:
         * - LC_LOAD_DYLINKER - Asks for dyld to be loaded (stub for now)
         * - LC_DYSYMTAB - Contains the dynamic symbol table for the binary (stub for now)
         * - LC_FUNCTION_STARTS - Contains the function start addresses for the binary (stub for now)
         * - LC_DATA_IN_CODE - Contains information about data in __TEXT segment (stub for now)
         * 
         * Implemented:
         * - LC_SEGMENT_64 - Contains a 64-bit segment of the binary (implemented)
         * - LC_SYMTAB - Contains the symbol table and string table for the binary (implemented)
         * - LC_MAIN - Contains the entry point command for the binary (implemented)
         */

        switch (lc->cmd) {
            case LC_SEGMENT_64: {
                const segment_command_64* seg = (const segment_command_64*)(lc);
                klog(KLOG_INFO, "macho-loader: Found segment: %s (vm_addr: 0x%p, size: 0x%p)\n",
                     seg->segname,
                     seg->vmaddr,
                     seg->vmsize);

                // Save __TEXT and __DATA segment information
                if (seg->segname[0] == '_' && seg->segname[1] == '_') {
                    if (seg->segname[2] == 'T') {  // __TEXT
                        out_info->text_vmaddr = seg->vmaddr;
                        out_info->text_vmsize = seg->vmsize;
                        out_info->text_fileoff = seg->fileoff;
                        out_info->text_filesize = seg->filesize;

                    } else if (seg->segname[2] == 'D') {  // __DATA
                        out_info->data_vmaddr = seg->vmaddr;
                        out_info->data_vmsize = seg->vmsize;
                        out_info->data_fileoff = seg->fileoff;
                        out_info->data_filesize = seg->filesize;
                    }
                }

                break;
            }
            case LC_SYMTAB: {
                klog(KLOG_INFO, "macho-loader: LC_SYMTAB command: stub\n");
                const symtab_command* symtab = (const symtab_command*)(lc);
                const struct nlist_64* symbols = (const struct nlist_64*)(data + symtab->symoff);
                const char* strtab = (const char*)(data + symtab->stroff);

                klog(KLOG_INFO, "macho-loader: Number of symbols: %u, string table size: %u\n", symtab->nsyms, symtab->strsize);

                break;
            }
            case LC_DYSYMTAB: {
                klog(KLOG_INFO, "macho-loader: LC_DYSYMTAB command: stub\n");
                // Handle dynamic symbol table command if needed
                break;
            }
            case LC_LOAD_DYLINKER: {
                klog(KLOG_INFO, "macho-loader: LC_LOAD_DYLINKER command: stub\n");
                // Handle load dylinker command if needed
                break;
            }
            case LC_FUNCTION_STARTS: {
                klog(KLOG_INFO, "macho-loader: LC_FUNCTION_STARTS command: stub\n");
                // Handle function starts command if needed
                break;
            }
            case LC_DATA_IN_CODE: {
                klog(KLOG_INFO, "macho-loader: LC_DATA_IN_CODE command: stub\n");
                // Handle data in code command if needed
                break;
            }
            case LC_MAIN: {
                const entry_point_command* ep = (const entry_point_command*)(lc);

                // Offset the entry point by the virtual memory address of the __TEXT segment
                out_info->entrypoint = out_info->text_vmaddr + ep->entryoff;

                klog(KLOG_INFO, "macho-loader: Entry point offset: 0x%p, entry point address: 0x%p\n", ep->entryoff, out_info->entrypoint);
                break;
            }
            default: {
                // Handle other load commands as needed

                const char* lcstr = macho_describe_lc(lc->cmd);
                klog(KLOG_WARN, "macho-loader: Unhandled load command: 0x%x (%s)\n", lc->cmd, lcstr);
                break;
            }
        }

        // Advance to the next load command
        load_cmd_ptr += lc->cmdsize;
    }
    return true;
}

bool macho_load(const uint8_t* data, page_table_t* root, uint64_t hhdm_base, struct macho_info* out_info) {
    // Parse the Mach-O header and load commands to extract segment information
    if (!macho_parse(data, out_info)) {
        return false;
    }

    // Allocate physical pages, zero BSS space, and map into page table
    if (!map_segment(root, out_info->text_vmaddr, out_info->text_vmsize, hhdm_base)) {
        return false;
    }
    if (!map_segment(root, out_info->data_vmaddr, out_info->data_vmsize, hhdm_base)) {
        return false;
    }

    // Resolve physical addresses via VMM and copy payload bytes using HHDM
    if (!copy_segment_payload(root, out_info->text_vmaddr, data, out_info->text_fileoff, out_info->text_filesize, hhdm_base) ||
        !copy_segment_payload(root, out_info->data_vmaddr, data, out_info->data_fileoff, out_info->data_filesize, hhdm_base)) {
        return false;
    }

    return true;
}