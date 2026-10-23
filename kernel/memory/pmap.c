#include "pmap.h"
#include "pmm.h"

#include <string.h>

static page_table_t *pml4;
static uint64_t pml4_phys_addr;
static uint64_t higher_half_base;

static void clear_page_table(page_table_t *table) {
    for (size_t i = 0; i < 512; ++i) {
        table->entries[i].value = 0;
    }
}

/**
 * Get or create the next level page table.
 */
static page_table_t *get_or_create_next_level(page_table_t *current, size_t index, uint64_t flags) {
    page_table_entry_t *entry = &current->entries[index];
    if (pte_is_present(entry)) {
        uint64_t next_level_phys = pte_get_address(entry);
        return (page_table_t *)(uintptr_t)(next_level_phys + higher_half_base);
    }

    void *new_frame;

    if (pmm_alloc_page(&new_frame) != 0) {
        return NULL;
    }

    uint64_t new_frame_phys = (uint64_t)(uintptr_t)new_frame;
    page_table_t *new_table = (page_table_t *)(uintptr_t)(new_frame_phys + higher_half_base);
    clear_page_table(new_table);

    // Set page table entry for the new table
    pte_set_address(entry, new_frame_phys);
    pte_set_flags(entry, PAGE_PRESENT | PAGE_READ_WRITE | flags);
    return new_table;
}

int pmap_initialize(uint64_t hhdm_base_offset) {
    uintptr_t active_cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(active_cr3));
    higher_half_base = hhdm_base_offset;
    pml4_phys_addr = active_cr3 & 0x000FFFFFFFFFF000ULL;
    pml4 = (page_table_t *)(uintptr_t)(pml4_phys_addr + higher_half_base);

    return 0;
}

int pmap_map(page_table_t *root, uint64_t virt, uint64_t phys, uint64_t flags) {
    if (root == NULL) {
        return -1;
    }
    size_t pml4_index = (size_t)((virt >> 39) & 0x1FF);
    size_t pdpt_index = (size_t)((virt >> 30) & 0x1FF);
    size_t pd_index = (size_t)((virt >> 21) & 0x1FF);
    size_t pt_index = (size_t)((virt >> 12) & 0x1FF);

    page_table_t *pdpt = get_or_create_next_level(root, pml4_index, flags);
    if (pdpt == NULL) {
        return -1;
    }
    page_table_t *pd = get_or_create_next_level(pdpt, pdpt_index, flags);
    if (pd == NULL) {
        return -1;
    }
    page_table_t *pt = get_or_create_next_level(pd, pd_index, flags);
    if (pt == NULL) {
        return -1;
    }

    page_table_entry_t *entry = &pt->entries[pt_index];
    pte_set_address(entry, phys);
    pte_set_flags(entry, PAGE_PRESENT | PAGE_READ_WRITE | flags);

    // Flush the TLB for the newly mapped virtual address.
    pmap_flush_tlb(virt);
    return 0;
}

int pmap_unmap(page_table_t *root, uint64_t virt) {
    if (root == NULL) {
        return -1;
    }
    size_t pml4_index = (size_t)((virt >> 39) & 0x1FF);
    size_t pdpt_index = (size_t)((virt >> 30) & 0x1FF);
    size_t pd_index = (size_t)((virt >> 21) & 0x1FF);
    size_t pt_index = (size_t)((virt >> 12) & 0x1FF);

    page_table_entry_t *entry = &root->entries[pml4_index];
    if (!pte_is_present(entry)) {
        return -1;
    }
    page_table_t *pdpt = (page_table_t *)(uintptr_t)(pte_get_address(entry) + higher_half_base);

    entry = &pdpt->entries[pdpt_index];
    if (!pte_is_present(entry)) {
        return -1;
    }
    page_table_t *pd = (page_table_t *)(uintptr_t)(pte_get_address(entry) + higher_half_base);

    entry = &pd->entries[pd_index];
    if (!pte_is_present(entry)) {
        return -1;
    }
    page_table_t *pt = (page_table_t *)(uintptr_t)(pte_get_address(entry) + higher_half_base);

    entry = &pt->entries[pt_index];
    if (!pte_is_present(entry)) {
        return -1;
    }
    entry->value = 0;

    // Flush the TLB for the unmapped virtual address.
    pmap_flush_tlb(virt);
    return 0;
}

page_table_t *pmap_create_user_pml4(uint64_t hhdm_base) {
    void *phys_page;
    if (pmm_alloc_page(&phys_page) != 0) {
        return NULL;
    }

    uint64_t pml4_phys = (uint64_t)phys_page;

    // Grab the higher-half direct mapping address of the newly allocated PML4 page.
    page_table_t *user_pml4 = (page_table_t *)(uintptr_t)(pml4_phys + hhdm_base);

    // Zero out lower half of the PML4 (indices 0 to 255), reserved for user-space
    memset(&user_pml4->entries, 0, sizeof(page_table_entry_t) * 256);

    // Copy the higher-half kernel mappings from the current PML4 to the new user PML4.
    page_table_t *kernel_pml4 = pmap_get_pml4();

    memcpy(&user_pml4->entries[256], &kernel_pml4->entries[256], sizeof(page_table_entry_t) * 256);

    // Return the newly allocated user PML4.
    return user_pml4;
}

int pmap_switch_pml4(page_table_t *new_pml4) {
    uintptr_t pml4_virtual = (uintptr_t)new_pml4;
    if (new_pml4 == NULL || pml4_virtual < higher_half_base) {
        return -1;
    }

    uint64_t pml4_physical = pml4_virtual - higher_half_base;
    if ((pml4_physical & (PMAP_PAGE_SIZE - 1)) != 0) {
        return -1;
    }

    __asm__ volatile("mov %0, %%cr3" : : "r"(pml4_physical) : "memory");
    return 0;
}

int pmap_flush_tlb(uint64_t virt) {
    __asm__ volatile("invlpg (%0)" : : "r"(virt) : "memory");
    return 0;
}

page_table_t *pmap_get_pml4(void) {
    return pml4;
}

uint64_t pmap_get_pml4_physical(void) {
    return pml4_phys_addr;
}

uintptr_t pmap_get_physical(page_table_t *pml4, uint64_t virt) {
    if (!pml4) {
        return 0;
    }

    // PML4: shifted by 39 bits to get the index
    // PDPT: shifted by 30 bits to get the index
    // PD: shifted by 21 bits to get the index
    // PT: shifted by 12 bits to get the index
    size_t pml4_index = (size_t)((virt >> 39) & 0x1FF);
    size_t pdpt_index = (size_t)((virt >> 30) & 0x1FF);
    size_t pd_index = (size_t)((virt >> 21) & 0x1FF);
    size_t pt_index = (size_t)((virt >> 12) & 0x1FF);

    page_table_entry_t *pml4_entry = &pml4->entries[pml4_index];
    if (!pte_is_present(pml4_entry)) {
        return 0;
    }
    page_table_t *pdpt =
        (page_table_t *)(uintptr_t)(pte_get_address(pml4_entry) + higher_half_base);
    page_table_entry_t *pdpt_entry = &pdpt->entries[pdpt_index];
    if (!pte_is_present(pdpt_entry)) {
        return 0;
    }
    page_table_t *pd = (page_table_t *)(uintptr_t)(pte_get_address(pdpt_entry) + higher_half_base);
    page_table_entry_t *pd_entry = &pd->entries[pd_index];
    if (!pte_is_present(pd_entry)) {
        return 0;
    }
    page_table_t *pt = (page_table_t *)(uintptr_t)(pte_get_address(pd_entry) + higher_half_base);
    page_table_entry_t *pt_entry = &pt->entries[pt_index];
    if (!pte_is_present(pt_entry)) {
        return 0;
    }
    return pte_get_address(pt_entry);
}

int pmap_copy_from_user(void *destination, uintptr_t source, size_t size) {
    // Validate the source address and destination buffer before copying from user space.

    // Define the user-space address limit (47-bit address space) and check if the destination
    // address and size are within this limit and the source buffer is valid.
    const uintptr_t user_limit = UINT64_C(1) << 47;
    if (source >= user_limit || size > user_limit - source || (size != 0 && destination == NULL)) {
        return -1;
    }

    // Save the current CR3 and get the root page table
    uintptr_t active_cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(active_cr3));

    // The root page table is obtained from the current CR3 register and mapped into the higher half
    page_table_t *root =
        (page_table_t *)((active_cr3 & UINT64_C(0x000FFFFFFFFFF000)) + higher_half_base);
    unsigned char *output = destination;

    while (size != 0) {
        page_table_t *table = root;
        uint64_t physical = 0;

        // Traverse the page table hierarchy to find the physical address corresponding to the
        // user-space source address
        for (unsigned int shift = 39;; shift -= 9) {
            // Shift indicates the current level in the page table hierarchy (39 for PML4, 30 for
            // PDPT, 21 for PD, 12 for PT)

            // Get the page table entry for the current level
            const page_table_entry_t *entry = &table->entries[(source >> shift) & 0x1FF];
            if (!pte_is_present(entry) || !pte_is_user(entry)) {
                return -1;
            }

            // If we reach the last level (PT) or encounter a huge page, calculate the physical
            // address
            if (shift == 12 || ((shift == 30 || shift == 21) && pte_is_huge_page(entry))) {
                uintptr_t offset_mask = (UINT64_C(1) << shift) - 1;
                physical = (pte_get_address(entry) & ~offset_mask) | (source & offset_mask);
                break;
            }
            if (shift == 39 && pte_is_huge_page(entry)) {
                return -1;
            }
            table = (page_table_t *)(pte_get_address(entry) + higher_half_base);
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

int pmap_copy_to_user(uintptr_t destination, const void *source, size_t size) {
    // Validate the destination address and source buffer before copying to user space.

    // Define the user-space address limit (47-bit address space) and check if the destination
    // address and size are within this limit and the source buffer is valid.
    const uintptr_t user_limit = UINT64_C(1) << 47;
    if (destination >= user_limit || size > user_limit - destination ||
        (size != 0 && source == NULL)) {
        return -1;
    }

    // Save the current CR3 register and get the root page table for the active address space.
    uintptr_t active_cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(active_cr3));
    page_table_t *root =
        (page_table_t *)((active_cr3 & UINT64_C(0x000FFFFFFFFFF000)) + higher_half_base);
    const unsigned char *input = source;

    while (size != 0) {
        page_table_t *table = root;
        uint64_t physical = 0;

        // Traverse the page table hierarchy to find the physical address corresponding to the
        // user-space source address
        for (unsigned int shift = 39;; shift -= 9) {
            const page_table_entry_t *entry = &table->entries[(destination >> shift) & 0x1FF];
            if (!pte_is_present(entry) || !pte_is_user(entry) || !pte_is_writable(entry)) {
                return -1;
            }
            if (shift == 12 || ((shift == 30 || shift == 21) && pte_is_huge_page(entry))) {
                uintptr_t offset_mask = (UINT64_C(1) << shift) - 1;
                physical = (pte_get_address(entry) & ~offset_mask) | (destination & offset_mask);
                break;
            }
            if (shift == 39 && pte_is_huge_page(entry)) {
                return -1;
            }
            table = (page_table_t *)(pte_get_address(entry) + higher_half_base);
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