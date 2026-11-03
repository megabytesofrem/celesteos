#include "pmm.h"

#include <error.h>

static uint8_t *bitmap;
static size_t totalPages;
static size_t usedPages;
static uint64_t higherHalfBase;

static void set_bit(size_t page) {
    bitmap[page / 8] |= (uint8_t)(1U << (page % 8));
}

static void clear_bit(size_t page) {
    bitmap[page / 8] &= (uint8_t)~(1U << (page % 8));
}

static bool test_bit(size_t page) {
    return (bitmap[page / 8] & (uint8_t)(1U << (page % 8))) != 0;
}

int KeInitPhysMemoryManager(struct limine_memmap_response *memmap, uint64_t hhdmBaseOffset) {
    uint64_t highestAddress = 0;
    uint64_t alignedBitmapSize;
    uint64_t bitmapPages;
    uint8_t *newBitmap = NULL;

    if (memmap == NULL || memmap->entries == NULL || memmap->entry_count == 0) {
        return ERR_INVALID_ARG;
    }

    for (uint64_t i = 0; i < memmap->entry_count; ++i) {
        struct limine_memmap_entry *entry = memmap->entries[i];
        if (entry != NULL && entry->type == LIMINE_MEMMAP_USABLE) {
            uint64_t endAddress = entry->base + entry->length;
            if (endAddress < entry->base) {
                return ERR_OVERFLOW;
            }
            if (endAddress > highestAddress) {
                highestAddress = endAddress;
            }
        }
    }

    if (highestAddress / PHYS_PAGE_SIZE > SIZE_MAX) {
        return ERR_OVERFLOW;
    }
    size_t newTotalPages = (size_t)(highestAddress / PHYS_PAGE_SIZE);
    if (newTotalPages > SIZE_MAX - 7) {
        return ERR_OVERFLOW;
    }
    size_t newBitmapSize = (newTotalPages + 7) / 8;
    if (newBitmapSize > SIZE_MAX - (PHYS_PAGE_SIZE - 1)) {
        return ERR_OVERFLOW;
    }

    bitmapPages = (newBitmapSize + PHYS_PAGE_SIZE - 1) / PHYS_PAGE_SIZE;
    alignedBitmapSize = bitmapPages * PHYS_PAGE_SIZE;

    for (uint64_t i = 0; i < memmap->entry_count; ++i) {
        struct limine_memmap_entry *entry = memmap->entries[i];
        if (entry != NULL && entry->type == LIMINE_MEMMAP_USABLE &&
            entry->length >= alignedBitmapSize && entry->base <= UINT64_MAX - hhdmBaseOffset) {
            newBitmap = (uint8_t *)(uintptr_t)(entry->base + hhdmBaseOffset);
            break;
        }
    }
    if (newBitmap == NULL) {
        return ERR_NO_MEMORY;
    }

    for (size_t i = 0; i < newBitmapSize; ++i) {
        newBitmap[i] = 0xFF;
    }

    bitmap = newBitmap;
    totalPages = newTotalPages;
    usedPages = 0;
    higherHalfBase = hhdmBaseOffset;

    for (uint64_t i = 0; i < memmap->entry_count; ++i) {
        struct limine_memmap_entry *entry = memmap->entries[i];
        if (entry != NULL && entry->type == LIMINE_MEMMAP_USABLE) {
            uint64_t endAddress = entry->base + entry->length;
            for (uint64_t addr = entry->base; addr < endAddress;) {
                size_t page = (size_t)(addr / PHYS_PAGE_SIZE);
                if (page < totalPages && test_bit(page)) {
                    clear_bit(page);
                }
                if (UINT64_MAX - addr < PHYS_PAGE_SIZE) {
                    break;
                }
                addr += PHYS_PAGE_SIZE;
            }
        }
    }

    uint64_t bitmapPhys = (uint64_t)(uintptr_t)bitmap - higherHalfBase;
    for (uint64_t addr = bitmapPhys; addr < bitmapPhys + alignedBitmapSize;
        addr += PHYS_PAGE_SIZE) {
        size_t page = (size_t)(addr / PHYS_PAGE_SIZE);
        if (page < totalPages && !test_bit(page)) {
            set_bit(page);
            ++usedPages;
        }
    }

    if (totalPages != 0 && !test_bit(0)) {
        set_bit(0);
        ++usedPages;
    }

    return 0;
}

int KeAllocPhysPage(void **page) {
    if (page == NULL) {
        return ERR_INVALID_ARG;
    }

    for (size_t i = 0; i < totalPages; ++i) {
        if (!test_bit(i)) {
            set_bit(i);
            ++usedPages;
            *page = (void *)(uintptr_t)(i * PHYS_PAGE_SIZE + higherHalfBase);
            return 0;
        }
    }

    // No free pages available
    return ERR_NO_MEMORY;
}

int KeAllocPhysPages(size_t count, void **outPages) {
    if (count == 0 || outPages == NULL) {
        return ERR_INVALID_ARG;
    }
    if (count == 1) {
        return KeAllocPhysPage(outPages);
    }

    size_t freeRun = 0;
    size_t startPage = 0;
    for (size_t page = 0; page < totalPages; ++page) {
        // Search for a contiguous run of free pages
        if (!test_bit(page)) {
            // Found a free page, increment the run length
            if (freeRun == 0) {
                startPage = page;
            }
            ++freeRun;

            // Check if the contiguous run has reached the desired count
            if (freeRun == count) {
                // Set the bits for the contiguous run of pages
                for (size_t allocated = startPage; allocated < startPage + count; ++allocated) {
                    set_bit(allocated);
                    ++usedPages;
                }

                // Return a pointer to the start of the pages
                *outPages = (void *)(uintptr_t)(startPage * PHYS_PAGE_SIZE + higherHalfBase);
                return 0;
            }
        } else {
            // Encountered an allocated page, reset the run length
            freeRun = 0;
        }
    }

    // No contiguous run of free pages found
    return ERR_NO_MEMORY;
}

int KeFreePhysPage(void *page) {
    if (page == NULL) {
        return ERR_INVALID_ARG;
    }

    size_t pageIndex = ((uintptr_t)page - higherHalfBase) / PHYS_PAGE_SIZE;
    if (pageIndex >= totalPages) {
        // Page index is out of range
        return ERR_INVALID_ARG;
    }

    if (pageIndex < totalPages && test_bit(pageIndex)) {
        // Clear the bit to mark the page as free
        clear_bit(pageIndex);
        --usedPages;
    }

    return 0;
}

int KeFreePhysPages(void *startPage, size_t count) {
    if (startPage == NULL) {
        return ERR_INVALID_ARG;
    }

    for (size_t i = 0; i < count; ++i) {
        // Grab the individual page from the contiguous block of pages
        void *page = (void *)((uintptr_t)startPage + i * PHYS_PAGE_SIZE);

        // Check against overflow
        if (i > (UINTPTR_MAX - (uintptr_t)startPage) / PHYS_PAGE_SIZE) {
            return ERR_OVERFLOW;
        }

        // Free the individual page using pmm_free_page
        int ret = KeFreePhysPage(page);
        if (ret != 0) {
            return ret;
        }
    }

    return 0;
}

size_t KeGetTotalPages() {
    return totalPages;
}

size_t KeGetUsedPages() {
    return usedPages;
}

size_t KeGetFreePages() {
    return totalPages - usedPages;
}

size_t KeGetTotalMemory() {
    return totalPages * PHYS_PAGE_SIZE;
}

size_t KeGetUsedMemory() {
    return usedPages * PHYS_PAGE_SIZE;
}

size_t KeGetFreeMemory() {
    return (totalPages - usedPages) * PHYS_PAGE_SIZE;
}