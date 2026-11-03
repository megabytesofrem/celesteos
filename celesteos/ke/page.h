#ifndef CELESTEOS_PMAP_H
#define CELESTEOS_PMAP_H

#include <error.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define PMAP_PAGE_SIZE 4096

typedef enum KePageFlags {
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
} KePageFlags;

typedef struct KePageTableEntry {
    uint64_t value;
} KePageTableEntry;

static inline int KeSetPteAddress(KePageTableEntry *entry, uint64_t physAddr) {
    if (!entry) {
        return ERR_INVALID_ARG;
    }

    entry->value = (entry->value & ~0x000FFFFFFFFFF000ULL) | (physAddr & 0x000FFFFFFFFFF000ULL);
    return 0;
}

static inline uint64_t KeGetPteAddress(KePageTableEntry *entry) {
    if (!entry) {
        return ERR_INVALID_ARG;
    }

    return entry->value & 0x000FFFFFFFFFF000ULL;
}

static inline uint64_t KeGetPtePhysAddress(KePageTableEntry *entry) {
    if (!entry) {
        return ERR_INVALID_ARG;
    }

    return entry->value & 0x000FFFFFFFFFF000ULL;
}

static inline int KeSetPteFlags(KePageTableEntry *entry, uint64_t flags) {
    if (!entry) {
        return ERR_INVALID_ARG;
    }

    entry->value |= flags;
    return 0;
}

static inline int KeClearPteFlags(KePageTableEntry *entry, uint64_t flags) {
    if (!entry) {
        return ERR_INVALID_ARG;
    }

    entry->value &= ~flags;
    return 0;
}

static inline bool KeIsPtePresent(KePageTableEntry *entry) {
    if (!entry) {
        return false;
    }

    return (entry->value & PAGE_PRESENT) != 0;
}

static inline bool KeIsUserPage(KePageTableEntry *entry) {
    if (!entry) {
        return false;
    }

    return (entry->value & PAGE_USER) != 0;
}

static inline bool KeIsReadWritePage(KePageTableEntry *entry) {
    if (!entry) {
        return false;
    }

    return (entry->value & PAGE_READ_WRITE) != 0;
}

static inline bool KeIsHugePage(KePageTableEntry *entry) {
    if (!entry) {
        return false;
    }

    return (entry->value & PAGE_HUGE_PAGE) != 0;
}

/**
 * Page table aligned to 4 KiB.
 */
typedef struct __attribute__((aligned(4096))) KePageTable {
    KePageTableEntry entries[512];
} KePageTable;

/**
 * Initialize the page map using the active bootloader page table.
 */
int KePageMapInit(uint64_t hhdmBaseOffset);

/**
 * Map a virtual address to a physical address with the specified flags.
 */
int KeMapPage(KePageTable *table, uint64_t virtAddr, uint64_t physAddr, uint64_t flags);

/**
 * Unmap a virtual address from the page table.
 */
int KeUnmapPage(KePageTable *table, uint64_t virtAddr);

/**
 * Create a new user-space PML4 page table.
 */
KePageTable *KeCreateUserPageTable(uint64_t hhdmBase);

/**
 * Switch the active PML4 page table to the specified new PML4.
 * CR3 holds the physical address of the new PML4.
 */
int KePageMapSwitchTo(KePageTable *newPml4);

/**
 * Invalidate page tables for the specified virtual address.
 */
int KePageMapInvalidate(uint64_t virtAddr);

/**
 * Translate a virtual address to its corresponding physical address using the specified
 * page table.
 */
uint64_t KeTranslatePhysAddress(KePageTable *table, uint64_t virtAddr);

/**
 * Get the kernel PML4's HHDM pointer and physical address saved by pmap_init.
 */
KePageTable *KeGetPageTable(void);
uint64_t KeGetPageTablePhys(void);

int KeCopyPageFromUser(void *destination, uintptr_t source, size_t size);
int KePageToUser(uintptr_t destination, const void *source, size_t size);

#endif // CELESTEOS_PMAP_H