#include "vmm.h"

#include "pmm.h"

static page_table_t* pml4;
static uint64_t pml4_phys_addr;
static uint64_t higher_half_base;

/**
 * Clear all entries in the given page table.
 */
static void clear_page_table(page_table_t* table) {
    for (size_t i = 0; i < 512; ++i) {
        table->entries[i].value = 0;
    }
}

/**
 * Get or create the next level page table.
 */
static page_table_t* get_or_create_next_level(page_table_t* current,
                                              size_t index,
                                              uint64_t flags) {
    page_table_entry_t* entry = &current->entries[index];
    if (pte_is_present(entry)) {
        uint64_t next_level_phys = pte_get_address(entry);
        return (page_table_t*)(uintptr_t)(next_level_phys + higher_half_base);
    }

    void* new_frame = pmm_alloc_page();
    if (new_frame == NULL) {
        return NULL;
    }

    uint64_t new_frame_phys = (uint64_t)(uintptr_t)new_frame;
    page_table_t* new_table =
        (page_table_t*)(uintptr_t)(new_frame_phys + higher_half_base);
    clear_page_table(new_table);

    pte_set_address(entry, new_frame_phys);
    pte_set_flags(entry, PAGE_PRESENT | PAGE_READ_WRITE | flags);
    return new_table;
}

bool vmm_initialize(uint64_t hhdm_base_offset) {
    uintptr_t active_cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(active_cr3));
    higher_half_base = hhdm_base_offset;
    pml4_phys_addr = active_cr3 & 0x000FFFFFFFFFF000ULL;
    pml4 = (page_table_t*)(uintptr_t)(pml4_phys_addr + higher_half_base);
    return true;
}

page_table_t* vmm_get_pml4(void) { return pml4; }

uint64_t vmm_get_pml4_physical(void) { return pml4_phys_addr; }

uintptr_t vmm_get_physical(page_table_t* pml4, uint64_t virt) {
    if (!pml4) {
        return 0;
    }
    size_t pml4_index = (size_t)((virt >> 39) & 0x1FF);
    size_t pdpt_index = (size_t)((virt >> 30) & 0x1FF);
    size_t pd_index = (size_t)((virt >> 21) & 0x1FF);
    size_t pt_index = (size_t)((virt >> 12) & 0x1FF);

    page_table_entry_t* pml4_entry = &pml4->entries[pml4_index];
    if (!pte_is_present(pml4_entry)) {
        return 0;
    }
    page_table_t* pdpt = (page_table_t*)(uintptr_t)(pte_get_address(pml4_entry) + higher_half_base);
    page_table_entry_t* pdpt_entry = &pdpt->entries[pdpt_index];
    if (!pte_is_present(pdpt_entry)) {
        return 0;
    }
    page_table_t* pd = (page_table_t*)(uintptr_t)(pte_get_address(pdpt_entry) + higher_half_base);
    page_table_entry_t* pd_entry = &pd->entries[pd_index];
    if (!pte_is_present(pd_entry)) {
        return 0;
    }
    page_table_t* pt = (page_table_t*)(uintptr_t)(pte_get_address(pd_entry) + higher_half_base);
    page_table_entry_t* pt_entry = &pt->entries[pt_index];
    if (!pte_is_present(pt_entry)) {
        return 0;
    }
    return pte_get_address(pt_entry);
}

bool vmm_map(page_table_t* root,
             uint64_t virt,
             uint64_t phys,
             uint64_t flags) {
    if (root == NULL) {
        return false;
    }
    size_t pml4_index = (size_t)((virt >> 39) & 0x1FF);
    size_t pdpt_index = (size_t)((virt >> 30) & 0x1FF);
    size_t pd_index = (size_t)((virt >> 21) & 0x1FF);
    size_t pt_index = (size_t)((virt >> 12) & 0x1FF);

    page_table_t* pdpt = get_or_create_next_level(root, pml4_index, flags);
    if (pdpt == NULL) {
        return false;
    }
    page_table_t* pd = get_or_create_next_level(pdpt, pdpt_index, flags);
    if (pd == NULL) {
        return false;
    }
    page_table_t* pt = get_or_create_next_level(pd, pd_index, flags);
    if (pt == NULL) {
        return false;
    }

    page_table_entry_t* entry = &pt->entries[pt_index];
    pte_set_address(entry, phys);
    pte_set_flags(entry, PAGE_PRESENT | PAGE_READ_WRITE | flags);

    // Flush the TLB for the newly mapped virtual address.
    vmm_flush_tlb(virt);
    return true;
}

void vmm_unmap(page_table_t* root, uint64_t virt) {
    if (root == NULL) {
        return;
    }
    size_t pml4_index = (size_t)((virt >> 39) & 0x1FF);
    size_t pdpt_index = (size_t)((virt >> 30) & 0x1FF);
    size_t pd_index = (size_t)((virt >> 21) & 0x1FF);
    size_t pt_index = (size_t)((virt >> 12) & 0x1FF);

    page_table_entry_t* entry = &root->entries[pml4_index];
    if (!pte_is_present(entry)) {
        return;
    }
    page_table_t* pdpt = (page_table_t*)(uintptr_t)(pte_get_address(entry) + higher_half_base);

    entry = &pdpt->entries[pdpt_index];
    if (!pte_is_present(entry)) {
        return;
    }
    page_table_t* pd = (page_table_t*)(uintptr_t)(pte_get_address(entry) + higher_half_base);

    entry = &pd->entries[pd_index];
    if (!pte_is_present(entry)) {
        return;
    }
    page_table_t* pt = (page_table_t*)(uintptr_t)(pte_get_address(entry) + higher_half_base);

    entry = &pt->entries[pt_index];
    if (!pte_is_present(entry)) {
        return;
    }
    entry->value = 0;

    // Flush the TLB for the unmapped virtual address.
    vmm_flush_tlb(virt);
}

void vmm_switch_pml4(page_table_t* new_pml4) {
    __asm__ volatile("mov %0, %%cr3" : : "r"(new_pml4));
}

void vmm_flush_tlb(uint64_t virt) {
    __asm__ volatile("invlpg (%0)" : : "r"(virt) : "memory");
}
