#include "page.h"

#include <error.h>
#include <ke/pmm.h>
#include <string.h>

/* Kernel page table (PML 4) */
static KePageTable *k_pml4;

static uint64_t pml4_phys_addr;
static uint64_t higher_half_base;

static void KeClearPageTable(KePageTable *table) {
    for (size_t i = 0; i < 512; ++i) {
        table->entries[i].value = 0;
    }
}

static KePageTable *KeGetOrCreateLevel(KePageTable *table, size_t index, uint64_t flags) {
    KePageTableEntry *entry = &table->entries[index];
    if (KeIsPtePresent(entry)) {
        uint64_t next_level_phys = KeGetPteAddress(entry);
        return (KePageTable *)(uintptr_t)(next_level_phys + higher_half_base);
    }

    void *new_frame;

    if (KeAllocPhysPage(&new_frame) != 0) {
        return NULL;
    }

    uint64_t new_frame_virtual = (uint64_t)(uintptr_t)new_frame;
    uint64_t new_frame_phys = new_frame_virtual - higher_half_base;
    KePageTable *new_table = (KePageTable *)(uintptr_t)new_frame_virtual;
    KeClearPageTable(new_table);

    // Set page table entry for the new table
    KeSetPteAddress(entry, new_frame_phys);
    KeSetPteFlags(entry, PAGE_PRESENT | PAGE_READ_WRITE | flags);
    return new_table;
}

int KePageMapInit(uint64_t hhdm_base_offset) {
    // Get CR3 which contains the physical address of the page table root
    uintptr_t cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));

    higher_half_base = hhdm_base_offset;
    pml4_phys_addr = cr3 & 0x000FFFFFFFFFF000ULL;
    k_pml4 = (KePageTable *)(uintptr_t)(pml4_phys_addr + higher_half_base);
    return 0;
}

KePageTable *KeGetPageTable(void) {
    return k_pml4;
}

uint64_t KeGetPageTablePhys(void) {
    return pml4_phys_addr;
}

int KeMapPage(KePageTable *table, uint64_t virt_addr, uint64_t phys_addr, uint64_t flags) {
    if (!table) {
        return ERR_INVALID_ARG;
    }

    size_t pml4_index = (size_t)((virt_addr >> 39) & 0x1FF);
    size_t pdpt_index = (size_t)((virt_addr >> 30) & 0x1FF);
    size_t pd_index = (size_t)((virt_addr >> 21) & 0x1FF);
    size_t pt_index = (size_t)((virt_addr >> 12) & 0x1FF);

    KePageTable *pdpt = KeGetOrCreateLevel(table, pml4_index, flags); // PDPT
    if (!pdpt) {
        return ERR_INVALID_STATE;
    }
    KePageTable *pd = KeGetOrCreateLevel(pdpt, pdpt_index, flags); // PD
    if (!pd) {
        return ERR_INVALID_STATE;
    }
    KePageTable *pt = KeGetOrCreateLevel(pd, pd_index, flags); // PT
    if (!pt) {
        return ERR_INVALID_STATE;
    }

    // Get the page table entry for the given virtual address
    KePageTableEntry *KePageTableEntry = &pt->entries[pt_index];

    // Set the physical address and flags and mark it as present and read/write
    KeSetPteAddress(KePageTableEntry, phys_addr);
    KeSetPteFlags(KePageTableEntry, PAGE_PRESENT | PAGE_READ_WRITE | flags);
    return 0;
}

int KeUnmapPage(KePageTable *table, uint64_t virt_addr) {
    if (!table) {
        return ERR_INVALID_ARG;
    }
    size_t pml4_index = (size_t)((virt_addr >> 39) & 0x1FF);
    size_t pdpt_index = (size_t)((virt_addr >> 30) & 0x1FF);
    size_t pd_index = (size_t)((virt_addr >> 21) & 0x1FF);
    size_t pt_index = (size_t)((virt_addr >> 12) & 0x1FF);

    KePageTableEntry *entry = &table->entries[pml4_index];
    if (!KeIsPtePresent(entry)) {
        return ERR_INVALID_ARG;
    }
    KePageTable *pdpt = (KePageTable *)(uintptr_t)(KeGetPteAddress(entry) + higher_half_base);

    entry = &pdpt->entries[pdpt_index];
    if (!KeIsPtePresent(entry)) {
        return ERR_INVALID_ARG;
    }
    KePageTable *pd = (KePageTable *)(uintptr_t)(KeGetPteAddress(entry) + higher_half_base);

    entry = &pd->entries[pd_index];
    if (!KeIsPtePresent(entry)) {
        return ERR_INVALID_ARG;
    }
    KePageTable *pt = (KePageTable *)(uintptr_t)(KeGetPteAddress(entry) + higher_half_base);

    entry = &pt->entries[pt_index];
    if (!KeIsPtePresent(entry)) {
        return ERR_INVALID_ARG;
    }
    entry->value = 0;

    // Invalidate the TLB for the unmapped virtual address.
    KePageMapInvalidate(virt_addr);
    return 0;
}

KePageTable *KeCreateUserPageTable(uint64_t hhdm_base) {
    if (hhdm_base != higher_half_base) {
        return NULL;
    }

    void *phys_page;
    if (KeAllocPhysPage(&phys_page) != 0) {
        return NULL;
    }

    // Grab the higher-half direct mapping address of the newly allocated PML4 page.
    KePageTable *user_pml4 = (KePageTable *)phys_page;

    // Zero out lower half of the PML4 (indices 0 to 255), reserved for user-space
    memset(&user_pml4->entries, 0, sizeof(KePageTableEntry) * 256);

    // Copy the higher-half kernel mappings from the current PML4 to the new user PML4.
    KePageTable *kernel_pml4 = KeGetPageTable();

    memcpy(&user_pml4->entries[256], &kernel_pml4->entries[256], sizeof(KePageTableEntry) * 256);

    // Return the newly allocated user PML4.
    return user_pml4;
}

int KePageMapSwitchTo(KePageTable *new_pml4) {
    uintptr_t pml4_virtual = (uintptr_t)new_pml4;
    if (new_pml4 == NULL || pml4_virtual < higher_half_base) {
        return ERR_INVALID_ARG;
    }

    uint64_t pml4_physical = pml4_virtual - higher_half_base;
    if ((pml4_physical & (PMAP_PAGE_SIZE - 1)) != 0) {
        return ERR_INVALID_ARG;
    }

    __asm__ volatile("mov %0, %%cr3" : : "r"(pml4_physical) : "memory");
    return 0;
}

int KePageMapInvalidate(uint64_t virt_addr) {
    __asm__ volatile("invlpg (%0)" : : "r"(virt_addr) : "memory");
    return 0;
}

uintptr_t KeTranslatePhysAddress(KePageTable *pml4, uint64_t virt_addr) {
    if (!pml4) {
        return 0;
    }

    // PML4: shifted by 39 bits to get the index
    // PDPT: shifted by 30 bits to get the index
    // PD: shifted by 21 bits to get the index
    // PT: shifted by 12 bits to get the index
    size_t pml4_index = (size_t)((virt_addr >> 39) & 0x1FF);
    size_t pdpt_index = (size_t)((virt_addr >> 30) & 0x1FF);
    size_t pd_index = (size_t)((virt_addr >> 21) & 0x1FF);
    size_t pt_index = (size_t)((virt_addr >> 12) & 0x1FF);

    KePageTableEntry *pml4_entry = &pml4->entries[pml4_index];
    if (!KeIsPtePresent(pml4_entry)) {
        return 0;
    }
    KePageTable *pdpt = (KePageTable *)(uintptr_t)(KeGetPteAddress(pml4_entry) + higher_half_base);
    KePageTableEntry *pdpt_entry = &pdpt->entries[pdpt_index];
    if (!KeIsPtePresent(pdpt_entry)) {
        return 0;
    }
    KePageTable *pd = (KePageTable *)(uintptr_t)(KeGetPteAddress(pdpt_entry) + higher_half_base);
    KePageTableEntry *pd_entry = &pd->entries[pd_index];
    if (!KeIsPtePresent(pd_entry)) {
        return 0;
    }
    KePageTable *pt = (KePageTable *)(uintptr_t)(KeGetPteAddress(pd_entry) + higher_half_base);
    KePageTableEntry *pt_entry = &pt->entries[pt_index];
    if (!KeIsPtePresent(pt_entry)) {
        return 0;
    }
    return KeGetPteAddress(pt_entry);
}

int KeCopyPageFromUser(void *destination, uintptr_t source, size_t size) {
    // Validate the source address and destination buffer before copying from user space.

    // Define the user-space address limit (47-bit address space) and check if the destination
    // address and size are within this limit and the source buffer is valid.
    const uintptr_t user_limit = UINT64_C(1) << 47;
    if (source >= user_limit || size > user_limit - source || (size != 0 && destination == NULL)) {
        return ERR_INVALID_ARG;
    }

    // Save the current CR3 and get the root page table
    uintptr_t active_cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(active_cr3));

    // The root page table is obtained from the current CR3 register and mapped into the higher half
    KePageTable *root =
        (KePageTable *)((active_cr3 & UINT64_C(0x000FFFFFFFFFF000)) + higher_half_base);
    unsigned char *output = destination;

    while (size != 0) {
        KePageTable *table = root;
        uint64_t physical = 0;

        // Traverse the page table hierarchy to find the physical address corresponding to the
        // user-space source address
        for (unsigned int shift = 39;; shift -= 9) {
            // Shift indicates the current level in the page table hierarchy (39 for PML4, 30 for
            // PDPT, 21 for PD, 12 for PT)

            // Get the page table entry for the current level
            KePageTableEntry *entry = &table->entries[(source >> shift) & 0x1FF];
            if (!KeIsPtePresent(entry) || !KeIsUserPage(entry) || !KeIsReadWritePage(entry)) {
                return ERR_INVALID_ARG;
            }

            // If we reach the last level (PT) or encounter a huge page, calculate the physical
            // address
            if (shift == 12 || ((shift == 30 || shift == 21) && KeIsHugePage(entry))) {
                uintptr_t offset_mask = (UINT64_C(1) << shift) - 1;
                physical = (KeGetPteAddress(entry) & ~offset_mask) | (source & offset_mask);
                break;
            }
            if (shift == 39 && KeIsHugePage(entry)) {
                return ERR_INVALID_ARG;
            }
            table = (KePageTable *)(KeGetPteAddress(entry) + higher_half_base);
        }

        // Calculate the size of the chunk to copy, ensuring it does not cross a page boundary
        size_t chunk = PMAP_PAGE_SIZE - (source & (PMAP_PAGE_SIZE - 1));
        if (chunk > size) {
            chunk = size;
        }

        // Copy the chunk from the physical address to the output buffer
        memcpy(output, (const void *)(physical + higher_half_base), chunk);
        output += chunk;
        source += chunk;
        size -= chunk;
    }
    return 0;
}

int KePageToUser(uintptr_t destination, const void *source, size_t size) {
    // Validate the destination address and source buffer before copying to user space.

    // Define the user-space address limit (47-bit address space) and check if the destination
    // address and size are within this limit and the source buffer is valid.
    const uintptr_t user_limit = UINT64_C(1) << 47;
    if (destination >= user_limit || size > user_limit - destination ||
        (size != 0 && source == NULL)) {
        return ERR_INVALID_ARG;
    }

    // Save the current CR3 register and get the root page table for the active address space.
    uintptr_t active_cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(active_cr3));
    KePageTable *root =
        (KePageTable *)((active_cr3 & UINT64_C(0x000FFFFFFFFFF000)) + higher_half_base);
    const unsigned char *input = source;

    while (size != 0) {
        KePageTable *table = root;
        uint64_t physical = 0;

        // Traverse the page table hierarchy to find the physical address corresponding to the
        // user-space source address
        for (unsigned int shift = 39;; shift -= 9) {
            KePageTableEntry *entry = &table->entries[(destination >> shift) & 0x1FF];
            if (!KeIsPtePresent(entry) || !KeIsUserPage(entry) || !KeIsReadWritePage(entry)) {
                return ERR_INVALID_ARG;
            }
            if (shift == 12 || ((shift == 30 || shift == 21) && KeIsHugePage(entry))) {
                uintptr_t offset_mask = (UINT64_C(1) << shift) - 1;
                physical = (KeGetPteAddress(entry) & ~offset_mask) | (destination & offset_mask);
                break;
            }
            if (shift == 39 && KeIsHugePage(entry)) {
                return ERR_INVALID_ARG;
            }
            table = (KePageTable *)(KeGetPteAddress(entry) + higher_half_base);
        }

        // Calculate the size of the chunk to copy, ensuring it does not cross a page boundary.
        size_t chunk = PMAP_PAGE_SIZE - (destination & (PMAP_PAGE_SIZE - 1));
        if (chunk > size) {
            chunk = size;
        }

        // Copy the chunk from the input buffer to the physical address in memory.
        memcpy((void *)(physical + higher_half_base), input, chunk);
        input += chunk;
        destination += chunk;
        size -= chunk;
    }
    return 0;
}