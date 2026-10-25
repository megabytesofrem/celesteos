#ifndef CELESTEOS_PMAP_H
#define CELESTEOS_PMAP_H

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define PMAP_PAGE_SIZE 4096

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

typedef struct pte {
    uint64_t value;
} pte_t;

static inline int pte_set_address(pte_t *entry, uint64_t phys_addr) {
    if (!entry) {
        return -EINVAL;
    }

    entry->value = (entry->value & ~0x000FFFFFFFFFF000ULL) | (phys_addr & 0x000FFFFFFFFFF000ULL);
    return 0;
}

static inline uint64_t pte_get_address(pte_t *entry) {
    if (!entry) {
        return -EINVAL;
    }

    return entry->value & 0x000FFFFFFFFFF000ULL;
}

static inline uint64_t pte_get_phys_address(pte_t *entry) {
    if (!entry) {
        return -EINVAL;
    }

    return entry->value & 0x000FFFFFFFFFF000ULL;
}

static inline int pte_set_flags(pte_t *entry, uint64_t flags) {
    if (!entry) {
        return -EINVAL;
    }

    entry->value |= flags;
    return 0;
}

static inline int pte_clear_flags(pte_t *entry, uint64_t flags) {
    if (!entry) {
        return -EINVAL;
    }

    entry->value &= ~flags;
    return 0;
}

static inline bool pte_is_present(pte_t *entry) {
    if (!entry) {
        return false;
    }

    return (entry->value & PAGE_PRESENT) != 0;
}

static inline bool pte_is_user(pte_t *entry) {
    if (!entry) {
        return false;
    }

    return (entry->value & PAGE_USER) != 0;
}

static inline bool pte_is_read_write(pte_t *entry) {
    if (!entry) {
        return false;
    }

    return (entry->value & PAGE_READ_WRITE) != 0;
}

static inline bool pte_is_huge_page(pte_t *entry) {
    if (!entry) {
        return false;
    }

    return (entry->value & PAGE_HUGE_PAGE) != 0;
}

/**
 * Page table aligned to 4 KiB.
 */
typedef struct __attribute__((aligned(4096))) page_table {
    pte_t entries[512];
} page_table_t;

/**
 * Initialize the page map using the active bootloader page table.
 */
int pmap_init(uint64_t hhdm_base_offset);

/**
 * Map a virtual address to a physical address with the specified flags.
 */
int pmap_map(page_table_t *table, uint64_t virt_addr, uint64_t phys_addr, uint64_t flags);

/**
 * Unmap a virtual address from the page table.
 */
int pmap_unmap(page_table_t *table, uint64_t virt_addr);

/**
 * Create a new user-space PML4 page table.
 */
page_table_t *pmap_create_user_pml4(uint64_t hhdm_base);

/**
 * Switch the active PML4 page table to the specified new PML4.
 * CR3 holds the physical address of the new PML4.
 */
int pmap_switch_pml4(page_table_t *new_pml4);

/**
 * Invalidate page tables for the specified virtual address.
 */
int pmap_invalidate_tlb(uint64_t virt_addr);

/**
 * Translate a virtual address to its corresponding physical address using the specified
 * page table.
 */
uint64_t pmap_translate_phys(page_table_t *table, uint64_t virt_addr);

/**
 * Get the kernel PML4's HHDM pointer and physical address saved by pmap_init.
 */
page_table_t *pmap_get_pml4(void);
uint64_t pmap_get_pml4_phys(void);

int pmap_copy_from_user(void *destination, uintptr_t source, size_t size);
int pmap_copy_to_user(uintptr_t destination, const void *source, size_t size);

#endif // CELESTEOS_PMAP_H