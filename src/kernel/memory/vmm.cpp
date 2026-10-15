#include "vmm.h"

#include <stdlib/string.h>

#include "pmm.h"

namespace memory {

VirtualMemoryManager& VirtualMemoryManager::instance() {
    static VirtualMemoryManager instance;
    return instance;
}

/**
 * Initialize the virtual memory manager.
 *
 * @param hhdm_base_offset The offset of the Higher Half Direct Mapping (HHDM) base address.
 */
void VirtualMemoryManager::initialize(size_t hhdm_base_offset) {
    this->higher_half_base = hhdm_base_offset;

    // Allocate a 4KiB page for the PML4
    void* pml4_frame = PhysMemoryManager::instance().alloc_page();
    this->pml4_phys_addr = reinterpret_cast<uint64_t>(pml4_frame);

    // Access the table in virtual memory via its higher half mapping
    this->pml4 = reinterpret_cast<PageTable*>(this->pml4_phys_addr +
                                              this->higher_half_base);

    // Zero out the PML4 to start with a clean slate
    memset(this->pml4, 0, VIRT_PAGE_SIZE);
}

PageTable* VirtualMemoryManager::get_or_create_next_level(PageTable* current,
                                                          size_t index,
                                                          uint64_t flags) {
    PageTableEntry& entry = current->entries[index];

    if (entry.is_present()) {
        // The next level already exists, so return its virtual address
        uint64_t next_level_phys = entry.get_address();
        return reinterpret_cast<PageTable*>(next_level_phys +
                                            this->higher_half_base);
    }

    // Allocate a new physical frame for the child table
    void* new_frame = PhysMemoryManager::instance().alloc_page();
    if (!new_frame) return nullptr;

    uint64_t new_frame_phys = reinterpret_cast<uint64_t>(new_frame);

    // Access and zero out table via higher-half mapped virtual address
    auto* new_table_virt =
        reinterpret_cast<PageTable*>(new_frame_phys + this->higher_half_base);
    memset(new_table_virt, 0, VIRT_PAGE_SIZE);

    entry.set_address(new_frame_phys);

    // Include ReadWrite so leaf PTEs control read/write policy
    entry.set_flags(static_cast<uint64_t>(PageFlags::Present) |
                    static_cast<uint64_t>(PageFlags::ReadWrite) | flags);

    return new_table_virt;
}

/**
 * Map a virtual address to a physical address in the page table, creating the next level
 * if needed.
 *
 * @param pml4 The top-level page table (PML4) to use for mapping.
 * @param virt The virtual address to map.
 * @param phys The physical address to map to.
 * @param flags The page table entry flags to set for the mapping.
 */
bool VirtualMemoryManager::map(PageTable* pml4,
                               uint64_t virt,
                               uint64_t phys,
                               uint64_t flags) {
    size_t pml4_index = (virt >> 39) & 0x1FF;
    size_t pdpt_index = (virt >> 30) & 0x1FF;
    size_t pd_index = (virt >> 21) & 0x1FF;
    size_t pt_index = (virt >> 12) & 0x1FF;

    PageTable* pdpt = get_or_create_next_level(pml4, pml4_index, flags);
    if (!pdpt) return false;

    PageTable* pd = get_or_create_next_level(pdpt, pdpt_index, flags);
    if (!pd) return false;

    PageTable* pt = get_or_create_next_level(pd, pd_index, flags);
    if (!pt) return false;

    // Map the page table entry to the target physical address
    PageTableEntry& entry = pt->entries[pt_index];
    entry.set_address(phys);

    // Set the page table entry flags to present and include any additional flags
    entry.set_flags(static_cast<uint64_t>(PageFlags::Present) | flags);

    // Flush the TLB for the virtual address being mapped
    flush_tlb(virt);
    return true;
}

/**
 * Unmap a virtual address from the page table.
 *
 * @param pml4 The top-level page table (PML4) to use for unmapping.
 * @param virt The virtual address to unmap.
 */
void VirtualMemoryManager::unmap(PageTable* pml4, uint64_t virt) {
    size_t pml4_index = (virt >> 39) & 0x1FF;
    size_t pdpt_index = (virt >> 30) & 0x1FF;
    size_t pd_index = (virt >> 21) & 0x1FF;
    size_t pt_index = (virt >> 12) & 0x1FF;

    if (!pml4->entries[pml4_index].is_present()) return;

    // Page Directory Pointer Table (PDPT)
    PageTable* pdpt = reinterpret_cast<PageTable*>(
        pml4->entries[pml4_index].get_address() + this->higher_half_base);
    if (!pdpt->entries[pdpt_index].is_present()) return;

    // Page Directory (PD)
    PageTable* pd = reinterpret_cast<PageTable*>(
        pdpt->entries[pdpt_index].get_address() + this->higher_half_base);
    if (!pd->entries[pd_index].is_present()) return;

    // Page Table (PT)
    PageTable* pt = reinterpret_cast<PageTable*>(
        pd->entries[pd_index].get_address() + this->higher_half_base);
    if (!pt->entries[pt_index].is_present()) return;

    // Clear the page table entry to unmap the virtual address
    pt->entries[pt_index].set_address(0);
    pt->entries[pt_index].set_flags(0);

    // Flush the TLB for the virtual address being unmapped
    flush_tlb(virt);
}

void VirtualMemoryManager::switch_pml4(PageTable* new_pml4) {
    // Load the new PML4 into the CR3 register to switch the page table
    asm volatile("mov %0, %%cr3" : : "r"(new_pml4));
}

}  // namespace memory