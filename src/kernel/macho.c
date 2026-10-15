#include "macho.h"

#include <stdbool.h>
#include <stdlib/libkernel.h>

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

bool macho_parse(const uint8_t* data, struct macho_info* out_info) {
    const mach_header_64* header = (const mach_header_64*)(data);
    if (header->magic != MH_MAGIC_64 && header->magic != MH_CIGAM_64) {
        klog(KLOG_ERROR, "macho: Invalid Mach-O magic number: %x\n", header->magic);
        return false;
    }

    // Advance to the first load command
    const uint8_t* load_cmd_ptr = data + sizeof(mach_header_64);

    // Read each load command
    for (uint32_t i = 0; i < header->ncmds; ++i) {
        const load_command* lc = (const load_command*)(load_cmd_ptr);

        switch (lc->cmd) {
            case LC_SEGMENT_64: {
                const segment_command_64* seg = (const segment_command_64*)(lc);
                klog(KLOG_INFO, "macho: Found segment: %s (vm_addr: 0x%p, size: 0x%p)\n",
                     seg->segname,
                     seg->vmaddr,
                     seg->vmsize);

                // Save __TEXT and __DATA segment information
                if (seg->segname[0] == '_' && seg->segname[1] == '_') {
                    if (seg->segname[2] == 'T') {  // __TEXT
                        out_info->text_vmaddr = seg->vmaddr;
                        out_info->text_size = seg->vmsize;
                    } else if (seg->segname[2] == 'D') {  // __DATA
                        out_info->data_vmaddr = seg->vmaddr;
                        out_info->data_size = seg->vmsize;
                    }
                }

                break;
            }
            case LC_MAIN: {
                const entry_point_command* ep = (const entry_point_command*)(lc);

                // Offset the entry point by the virtual memory address of the __TEXT segment
                out_info->entrypoint = out_info->text_vmaddr + ep->entryoff;

                klog(KLOG_INFO, "macho: Entry point offset: 0x%p, entry point address: 0x%p\n", ep->entryoff, out_info->entrypoint);
                break;
            }
            default: {
                // Handle other load commands as needed

                const char* lcstr = macho_describe_lc(lc->cmd);
                klog(KLOG_INFO, "macho: Unhandled load command: 0x%x (%s)\n", lc->cmd, lcstr);
                break;
            }
        }

        // Advance to the next load command
        load_cmd_ptr += lc->cmdsize;
    }
    return true;
}
