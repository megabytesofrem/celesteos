#include "pmm.h"

#include <errno.h>

static uint8_t *bitmap;
static size_t total_pages;
static size_t used_pages;
static uint64_t higher_half_base;

static void set_bit(size_t page) {
    bitmap[page / 8] |= (uint8_t)(1U << (page % 8));
}

static void clear_bit(size_t page) {
    bitmap[page / 8] &= (uint8_t)~(1U << (page % 8));
}

static bool test_bit(size_t page) {
    return (bitmap[page / 8] & (uint8_t)(1U << (page % 8))) != 0;
}

int pmm_init(struct limine_memmap_response *memmap, uint64_t hhdm_base_offset) {
    uint64_t highest_address = 0;
    uint64_t aligned_bitmap_size;
    uint64_t bitmap_pages;
    uint8_t *new_bitmap = NULL;

    if (memmap == NULL || memmap->entries == NULL || memmap->entry_count == 0) {
        return -EINVAL;
    }

    for (uint64_t i = 0; i < memmap->entry_count; ++i) {
        struct limine_memmap_entry *entry = memmap->entries[i];
        if (entry != NULL && entry->type == LIMINE_MEMMAP_USABLE) {
            uint64_t end_address = entry->base + entry->length;
            if (end_address < entry->base) {
                return -EOVERFLOW;
            }
            if (end_address > highest_address) {
                highest_address = end_address;
            }
        }
    }

    if (highest_address / PHYS_PAGE_SIZE > SIZE_MAX) {
        return -EOVERFLOW;
    }
    size_t new_total_pages = (size_t)(highest_address / PHYS_PAGE_SIZE);
    if (new_total_pages > SIZE_MAX - 7) {
        return -EOVERFLOW;
    }
    size_t new_bitmap_size = (new_total_pages + 7) / 8;
    if (new_bitmap_size > SIZE_MAX - (PHYS_PAGE_SIZE - 1)) {
        return -EOVERFLOW;
    }

    bitmap_pages = (new_bitmap_size + PHYS_PAGE_SIZE - 1) / PHYS_PAGE_SIZE;
    aligned_bitmap_size = bitmap_pages * PHYS_PAGE_SIZE;

    for (uint64_t i = 0; i < memmap->entry_count; ++i) {
        struct limine_memmap_entry *entry = memmap->entries[i];
        if (entry != NULL && entry->type == LIMINE_MEMMAP_USABLE &&
            entry->length >= aligned_bitmap_size && entry->base <= UINT64_MAX - hhdm_base_offset) {
            new_bitmap = (uint8_t *)(uintptr_t)(entry->base + hhdm_base_offset);
            break;
        }
    }
    if (new_bitmap == NULL) {
        return -ENOMEM;
    }

    for (size_t i = 0; i < new_bitmap_size; ++i) {
        new_bitmap[i] = 0xFF;
    }

    bitmap = new_bitmap;
    total_pages = new_total_pages;
    used_pages = 0;
    higher_half_base = hhdm_base_offset;

    for (uint64_t i = 0; i < memmap->entry_count; ++i) {
        struct limine_memmap_entry *entry = memmap->entries[i];
        if (entry != NULL && entry->type == LIMINE_MEMMAP_USABLE) {
            uint64_t end_address = entry->base + entry->length;
            for (uint64_t addr = entry->base; addr < end_address;) {
                size_t page = (size_t)(addr / PHYS_PAGE_SIZE);
                if (page < total_pages && test_bit(page)) {
                    clear_bit(page);
                }
                if (UINT64_MAX - addr < PHYS_PAGE_SIZE) {
                    break;
                }
                addr += PHYS_PAGE_SIZE;
            }
        }
    }

    uint64_t bitmap_phys = (uint64_t)(uintptr_t)bitmap - higher_half_base;
    for (uint64_t addr = bitmap_phys; addr < bitmap_phys + aligned_bitmap_size;
        addr += PHYS_PAGE_SIZE) {
        size_t page = (size_t)(addr / PHYS_PAGE_SIZE);
        if (page < total_pages && !test_bit(page)) {
            set_bit(page);
            ++used_pages;
        }
    }

    if (total_pages != 0 && !test_bit(0)) {
        set_bit(0);
        ++used_pages;
    }

    return 0;
}

int pmm_alloc_page(void **page) {
    if (page == NULL) {
        return -EINVAL;
    }

    for (size_t i = 0; i < total_pages; ++i) {
        if (test_bit(i)) {
            clear_bit(i);
            ++used_pages;
            *page = (void *)(uintptr_t)(i * PHYS_PAGE_SIZE + higher_half_base);
            return 0;
        }
    }

    // No free pages available
    return -ENOMEM;
}

int pmm_alloc_pages(size_t count, void **out_pages) {
    if (count == 0) {
        return -EINVAL;
    }
    if (count == 1) {
        return pmm_alloc_page(out_pages);
    }

    size_t free_run = 0;
    size_t start_page = 0;
    for (size_t page = 0; page < total_pages; ++page) {
        // Search for a contiguous run of free pages
        if (!test_bit(page)) {
            // Found a free page, increment the run length
            if (free_run == 0) {
                start_page = page;
            }
            ++free_run;

            // Check if the contiguous run has reached the desired count
            if (free_run == count) {
                // Set the bits for the contiguous run of pages
                for (size_t allocated = start_page; allocated < start_page + count; ++allocated) {
                    set_bit(allocated);
                    ++used_pages;
                }

                // Return a pointer to the start of the pages
                *out_pages = (void *)(uintptr_t)(start_page * PHYS_PAGE_SIZE);
                return 0;
            }
        } else {
            // Encountered an allocated page, reset the run length
            free_run = 0;
        }
    }

    // No contiguous run of free pages found
    return -ENOMEM;
}

int pmm_free_page(void *page) {
    if (page == NULL) {
        return -EINVAL;
    }

    size_t page_index = ((uintptr_t)page - higher_half_base) / PHYS_PAGE_SIZE;
    if (page_index >= total_pages) {
        // Page index is out of range
        return -EINVAL;
    }

    if (page_index < total_pages && test_bit(page_index)) {
        // Clear the bit to mark the page as free
        clear_bit(page_index);
        --used_pages;
    }

    return 0;
}

int pmm_free_pages(void *start_page, size_t count) {
    if (start_page == NULL) {
        return -EINVAL;
    }

    for (size_t i = 0; i < count; ++i) {
        // Grab the individual page from the contiguous block of pages
        void *page = (void *)((uintptr_t)start_page + i * PHYS_PAGE_SIZE);

        // Check against overflow
        if (i > (UINTPTR_MAX - (uintptr_t)start_page) / PHYS_PAGE_SIZE) {
            return -EOVERFLOW;
        }

        // Free the individual page using pmm_free_page
        int ret = pmm_free_page(page);
        if (ret != 0) {
            return ret;
        }
    }

    return 0;
}

size_t pmm_get_total_pages() {
    return total_pages;
}

size_t pmm_get_used_pages() {
    return used_pages;
}

size_t pmm_get_free_pages() {
    return total_pages - used_pages;
}

size_t pmm_get_total_memory() {
    return total_pages * PHYS_PAGE_SIZE;
}

size_t pmm_get_used_memory() {
    return used_pages * PHYS_PAGE_SIZE;
}

size_t pmm_get_free_memory() {
    return (total_pages - used_pages) * PHYS_PAGE_SIZE;
}