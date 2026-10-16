#ifndef CELESTEOS_VMM_H
#define CELESTEOS_VMM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <mach/kern_return.h>

#define VMM_PAGE_SIZE 0x1000 /* 4KiB */

typedef enum page_flags {
    PAGE_PRESENT = 1ULL << 0,
    PAGE_READ_WRITE = 1ULL << 1,
    PAGE_USER = 1ULL << 2,
    PAGE_WRITE_THROUGH = 1ULL << 3,
    PAGE_CACHE_DISABLED = 1ULL << 4,
    PAGE_ACCESSED = 1ULL << 5,
    PAGE_DIRTY = 1ULL << 6,
    PAGE_HUGE_PAGE = 1ULL << 7,
    PAGE_GLOBAL = 1ULL << 8,
    PAGE_NO_EXECUTE = 1ULL << 9
} page_flags;

/**
 * A single entry in the page table.
 */
typedef struct page_table_entry_t {
    uint64_t value;
} page_table_entry_t;

/**
 * Set the physical address for a page table entry.
 */
static inline kern_return_t pte_set_address(page_table_entry_t* entry,
                                            uint64_t phys_addr) {
    entry->value = (entry->value & ~0x000FFFFFFFFFF000ULL) |
                   (phys_addr & 0x000FFFFFFFFFF000ULL);
    return KERN_SUCCESS;
}

/**
 * Get the physical address from a page table entry.
 */
static inline uint64_t pte_get_address(
    const page_table_entry_t* entry) {
    return entry->value & 0x000FFFFFFFFFF000ULL;
}

/**
 * Set the flags for a page table entry.
 */
static inline kern_return_t pte_set_flags(page_table_entry_t* entry,
                                          uint64_t flags) {
    entry->value |= flags;
    return KERN_SUCCESS;
}

/**
 * Clear the specified flags for a page table entry.
 */
static inline kern_return_t pte_clear_flags(page_table_entry_t* entry,
                                            uint64_t flags) {
    entry->value &= ~flags;
    return KERN_SUCCESS;
}

/**
 * Check if a page table entry is present.
 */
static inline bool pte_is_present(const page_table_entry_t* entry) {
    return (entry->value & PAGE_PRESENT) != 0;
}

/**
 * Check if a page table entry is a user entry.
 */
static inline bool pte_is_user(const page_table_entry_t* entry) {
    return (entry->value & PAGE_USER) != 0;
}

/**
 * Check if a page table entry is writable.
 */
static inline bool pte_is_writable(const page_table_entry_t* entry) {
    return (entry->value & PAGE_READ_WRITE) != 0;
}

/**
 * Check if a page table entry is a huge page.
 */
static inline bool pte_is_huge_page(
    const page_table_entry_t* entry) {
    return (entry->value & PAGE_HUGE_PAGE) != 0;
}

/**
 * Page table aligned to 4096 bytes (4KiB) containing 512 entries.
 */
typedef struct page_table_t {
    page_table_entry_t entries[512];
} page_table_t __attribute__((aligned(4096)));

/**
 * Initialize the virtual memory manager using the active bootloader page table.
 */
kern_return_t vmm_initialize(uint64_t hhdm_base_offset);

/**
 * Map a virtual address to a physical address, creating the necessary levels
 * of the page table hierarchy if they do not already exist.
 */
kern_return_t vmm_map(page_table_t* pml4, uint64_t virt, uint64_t phys, uint64_t flags);

/**
 * Unmap a virtual address, removing its mapping from the page table hierarchy.
 */
kern_return_t vmm_unmap(page_table_t* pml4, uint64_t virt);

kern_return_t vmm_switch_pml4(page_table_t* new_pml4);
kern_return_t vmm_flush_tlb(uint64_t virt);

page_table_t* vmm_get_pml4(void);
uint64_t vmm_get_pml4_physical(void);
uintptr_t vmm_get_physical(page_table_t* pml4, uint64_t virt);

#endif  // CELESTEOS_VMM_H
