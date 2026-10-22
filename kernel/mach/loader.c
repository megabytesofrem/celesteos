#include "loader.h"

#include <libkernel.h>
#include <stdbool.h>
#include <string.h>

#include <kernel/memory/pmap.h>
#include <kernel/memory/pmm.h>
#include <kernel/memory/vm_map.h>

/**
 * __TEXT is readable (PAGE_PRESENT) and executable (PAGE_EXECUTE), but not writable.
 * __DATA is readable (PAGE_PRESENT) and writable (PAGE_READ_WRITE), but not executable.
 *
 * __LINKEDIT contains data used by the linker, typically read-only. Its contents are described by
 * load commands e.g LC_SYMTAB, LC_DYSYMTAB, etc.
 */
const char *macho_describe_lc(uint32_t lc) {
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
    return (size + PMAP_PAGE_SIZE - 1) & ~((size_t)PMAP_PAGE_SIZE - 1);
}

static inline size_t page_align_down(uintptr_t addr) {
    return addr & ~((uintptr_t)PMAP_PAGE_SIZE - 1);
}

/* Converts Mach-O VM protection flags to CelesteOS pmap flags */
static uint64_t macho_prot_to_page_flags(uint32_t prot) {
    uint64_t flags = PAGE_PRESENT | PAGE_USER;

    if (prot & VM_PROT_WRITE) {
        flags |= PAGE_READ_WRITE;
    }
    if (!(prot & VM_PROT_EXEC)) {
        flags |= PAGE_NO_EXECUTE;
    }
    return flags;
}

kern_return_t map_segment(page_table_t *root, uint64_t vmaddr, uint64_t vmsize, uint32_t initprot,
                          uint64_t hhdm_base) {
    if (vmsize == 0)
        return KERN_SUCCESS;

    uintptr_t vm_start = page_align_down(vmaddr);
    size_t offset = vmaddr - vm_start;
    if (vmsize > SIZE_MAX - offset - (PMAP_PAGE_SIZE - 1) || vmaddr > UINT64_MAX - vmsize) {
        klog(KLOG_ERROR, "macho-loader: Segment address range overflows.\n");
        return KERN_FAILURE;
    }
    size_t page_count = page_align_up(offset + vmsize) / PMAP_PAGE_SIZE;

    // Allocate backing physical pages
    void *phys_pages;
    if (pmm_alloc_pages(page_count, &phys_pages) != 0) {
        klog(KLOG_ERROR, "macho-loader: Failed physical allocation for segment at 0x%p\n",
             (void *)vmaddr);
        return KERN_FAILURE;
    }

    uintptr_t phys_addr = (uintptr_t)phys_pages;

    // Zero out physical memory via HHDM (clears uninitialized BSS)
    memset((void *)(phys_addr + hhdm_base), 0, page_count * PMAP_PAGE_SIZE);

    // Map physical frames into user virtual space with appropriate flags
    uint64_t pmap_flags = macho_prot_to_page_flags(initprot);

    for (size_t i = 0; i < page_count; ++i) {
        if (pmap_map(root, vm_start + (i * PMAP_PAGE_SIZE), phys_addr + (i * PMAP_PAGE_SIZE),
                     pmap_flags) != 0) {
            klog(KLOG_ERROR, "macho-loader: Failed to map page %zu for segment at 0x%p\n", i,
                 (void *)vmaddr);
            return KERN_FAILURE;
        }
    }
    return KERN_SUCCESS;
}

kern_return_t copy_segment_payload(page_table_t *root, uint64_t vmaddr, const uint8_t *payload,
                                   uint64_t fileoff, uint64_t filesz, uint64_t hhdm_base) {
    if (filesz == 0)
        return KERN_SUCCESS;

    size_t copied = 0;
    while (copied < filesz) {
        uintptr_t curr_user_addr = vmaddr + copied;
        uintptr_t phys_addr = pmap_get_physical(root, curr_user_addr);

        if (!phys_addr) {
            klog(KLOG_ERROR, "macho-loader: Failed to resolve physical frame for 0x%p\n",
                 (void *)curr_user_addr);
            return KERN_FAILURE;
        }

        size_t page_offset = curr_user_addr & (PMAP_PAGE_SIZE - 1);
        size_t chunk_size = PMAP_PAGE_SIZE - page_offset;
        if (chunk_size > (filesz - copied)) {
            chunk_size = filesz - copied;
        }

        // Copy directly into physical frame using HHDM
        memcpy((void *)(phys_addr + hhdm_base + page_offset), payload + fileoff + copied,
               chunk_size);
        copied += chunk_size;
    }
    return KERN_SUCCESS;
}

/* Allocates and maps a 16KiB Ring 3 User Stack */
uint64_t user_stack_allocate(page_table_t *root, uint64_t hhdm_base) {
    // User stack base address
    uint64_t stack_base = 0x00007FFFF0000000ULL;

    size_t page_count = 4; // 16 KiB

    void *phys_pages;
    if (pmm_alloc_pages(page_count, &phys_pages) != 0) {
        klog(KLOG_ERROR, "macho-loader: Failed to allocate physical pages for user stack!\n");
        return 0;
    }

    uintptr_t phys_addr = (uintptr_t)phys_pages;
    memset((void *)(phys_addr + hhdm_base), 0, page_count * PMAP_PAGE_SIZE);

    for (size_t i = 0; i < page_count; i++) {
        uint64_t virt = stack_base + (i * PMAP_PAGE_SIZE);
        uint64_t phys = phys_addr + (i * PMAP_PAGE_SIZE);

        if (pmap_map(root, virt, phys, PAGE_PRESENT | PAGE_USER | PAGE_READ_WRITE) != 0) {
            klog(KLOG_ERROR, "macho-loader: Failed to map user stack page at 0x%p\n", (void *)virt);
            return 0;
        }
    }

    // Return top of stack (stack grows downwards in x86_64)
    return stack_base + (page_count * PMAP_PAGE_SIZE);
}

kern_return_t macho_load(const uint8_t *data, page_table_t *root, uint64_t hhdm_base,
                         struct macho_info *out_info) {
    *out_info = (struct macho_info){ 0 };
    const struct mach_header_64 *header = (const struct mach_header_64 *)(data);

    if (header->magic != MH_MAGIC_64 && header->magic != MH_CIGAM_64) {
        klog(KLOG_ERROR, "macho-loader: Invalid Mach-O magic number: 0x%x\n", header->magic);
        return KERN_FAILURE;
    }

    bool has_lc_main = false;

    const uint8_t *load_cmd_ptr = data + sizeof(struct mach_header_64);
    uint64_t text_vmaddr = 0;
    uint64_t entry_offset = 0;

    for (uint32_t i = 0; i < header->ncmds; ++i) {
        const struct load_command *lc = (const struct load_command *)(load_cmd_ptr);

        switch (lc->cmd) {
        case LC_SEGMENT_64: {
            const struct segment_command_64 *seg = (const struct segment_command_64 *)(lc);

            // ALWAYS skip __PAGEZERO to avoid zero-page traps or 4GB memory
            // allocations!
            if (strcmp(seg->segname, "__PAGEZERO") == 0 || seg->vmsize == 0) {
                break;
            }

            klog(KLOG_INFO, "macho-loader: Mapping segment %s (vmaddr: 0x%p, size: 0x%p)\n",
                 seg->segname, (void *)seg->vmaddr, (void *)seg->vmsize);

            if (strcmp(seg->segname, "__TEXT") == 0) {
                text_vmaddr = seg->vmaddr;
            }

            // Allocate physical pages & map into PML4
            if (map_segment(root, seg->vmaddr, seg->vmsize, seg->initprot, hhdm_base) !=
                KERN_SUCCESS) {
                return KERN_FAILURE;
            }

            // Copy binary payload into mapped physical memory
            if (copy_segment_payload(root, seg->vmaddr, data, seg->fileoff, seg->filesize,
                                     hhdm_base) != KERN_SUCCESS) {
                return KERN_FAILURE;
            }

            break;
        }
        case LC_LOAD_DYLINKER: {
            // Map the dynamic linker segment into memory
            const struct dylinker_command *dyld = (const struct dylinker_command *)(lc);
            // if (map_segment(root, dyld->vmaddr, dyld->vmsize, dyld->initprot, hhdm_base) !=
            //     KERN_SUCCESS) {
            //     return KERN_FAILURE;
            // }
            // if (copy_segment_payload(root, dyld->vmaddr, data, dyld->fileoff, dyld->filesize,
            //                          hhdm_base) != KERN_SUCCESS) {
            //     return KERN_FAILURE;
            // }
            break;
        }
        case LC_MAIN: {
            const struct entry_point_command *ep = (const struct entry_point_command *)(lc);
            // entryoff is relative to the Mach-O header / __TEXT segment start
            entry_offset = ep->entryoff;
            has_lc_main = true;
            break;
        }
        case LC_UNIXTHREAD: {
            // OS X 10.6 legacy thread state entrypoint fallback
            const uint32_t *thread_state = (const uint32_t *)(load_cmd_ptr + 16);
            // x86_THREAD_STATE64 RIP field location
            uint64_t rip = *(const uint64_t *)&thread_state[16];
            if (!has_lc_main) {
                out_info->entrypoint = rip;
            }
            break;
        }
        default:
            break;
        }

        load_cmd_ptr += lc->cmdsize;
    }

    // Calculate final entry point virtual address
    out_info->entrypoint = text_vmaddr + entry_offset;
    klog(KLOG_INFO, "macho-loader: Resolved binary entrypoint at 0x%p\n",
         (void *)out_info->entrypoint);

    // Allocate Ring 3 user stack
    out_info->stack_pointer = user_stack_allocate(root, hhdm_base);
    if (out_info->stack_pointer == 0) {
        klog(KLOG_ERROR, "macho-loader: Failed to allocate user stack!\n");
        return KERN_FAILURE;
    }

    return KERN_SUCCESS;
}