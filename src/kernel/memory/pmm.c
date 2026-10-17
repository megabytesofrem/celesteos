#include "pmm.h"

#include <limits.h>

static uint8_t *bitmap;
static size_t total_pages;
static size_t used_pages;
static uint64_t higher_half_base;

static void set_bit(size_t page) { bitmap[page / 8] |= (uint8_t)(1U << (page % 8)); }

static void clear_bit(size_t page) { bitmap[page / 8] &= (uint8_t)~(1U << (page % 8)); }

static bool test_bit(size_t page) { return (bitmap[page / 8] & (uint8_t)(1U << (page % 8))) != 0; }

void pmm_initialize(struct limine_memmap_response *memmap, uint64_t hhdm_base_offset) {
    uint64_t highest_address = 0;
    uint64_t aligned_bitmap_size;
    uint64_t bitmap_pages;
    uint8_t *new_bitmap = NULL;

    if (memmap == NULL || memmap->entries == NULL || memmap->entry_count == 0) {
        return;
    }

    for (uint64_t i = 0; i < memmap->entry_count; ++i) {
        struct limine_memmap_entry *entry = memmap->entries[i];
        if (entry != NULL && entry->type == LIMINE_MEMMAP_USABLE) {
            uint64_t end_address = entry->base + entry->length;
            if (end_address < entry->base) {
                return;
            }
            if (end_address > highest_address) {
                highest_address = end_address;
            }
        }
    }

    if (highest_address / PMM_PAGE_SIZE > SIZE_MAX) {
        return;
    }
    size_t new_total_pages = (size_t)(highest_address / PMM_PAGE_SIZE);
    if (new_total_pages > SIZE_MAX - 7) {
        return;
    }
    size_t new_bitmap_size = (new_total_pages + 7) / 8;
    if (new_bitmap_size > SIZE_MAX - (PMM_PAGE_SIZE - 1)) {
        return;
    }

    bitmap_pages = (new_bitmap_size + PMM_PAGE_SIZE - 1) / PMM_PAGE_SIZE;
    aligned_bitmap_size = bitmap_pages * PMM_PAGE_SIZE;

    for (uint64_t i = 0; i < memmap->entry_count; ++i) {
        struct limine_memmap_entry *entry = memmap->entries[i];
        if (entry != NULL && entry->type == LIMINE_MEMMAP_USABLE &&
            entry->length >= aligned_bitmap_size && entry->base <= UINT64_MAX - hhdm_base_offset) {
            new_bitmap = (uint8_t *)(uintptr_t)(entry->base + hhdm_base_offset);
            break;
        }
    }
    if (new_bitmap == NULL) {
        return;
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
                size_t page = (size_t)(addr / PMM_PAGE_SIZE);
                if (page < total_pages && test_bit(page)) {
                    clear_bit(page);
                }
                if (UINT64_MAX - addr < PMM_PAGE_SIZE) {
                    break;
                }
                addr += PMM_PAGE_SIZE;
            }
        }
    }

    uint64_t bitmap_phys = (uint64_t)(uintptr_t)bitmap - higher_half_base;
    for (uint64_t addr = bitmap_phys; addr < bitmap_phys + aligned_bitmap_size;
         addr += PMM_PAGE_SIZE) {
        size_t page = (size_t)(addr / PMM_PAGE_SIZE);
        if (page < total_pages && !test_bit(page)) {
            set_bit(page);
            ++used_pages;
        }
    }

    if (total_pages != 0 && !test_bit(0)) {
        set_bit(0);
        ++used_pages;
    }

    return;
}

int pmm_alloc_page(void **out_page) {
    for (size_t page = 0; page < total_pages; ++page) {
        if (!test_bit(page)) {
            set_bit(page);
            ++used_pages;

            *out_page = (void *)(uintptr_t)(page * PMM_PAGE_SIZE);
            return 0;
        }
    }
    return -1;
}

int pmm_alloc_pages(size_t count, void **out_pages) {
    if (count == 0) {
        return -1;
    }
    if (count == 1) {
        return pmm_alloc_page(out_pages);
    }

    size_t free_run = 0;
    size_t start_page = 0;
    for (size_t page = 0; page < total_pages; ++page) {
        if (!test_bit(page)) {
            if (free_run == 0) {
                start_page = page;
            }
            ++free_run;
            if (free_run == count) {
                for (size_t allocated = start_page; allocated < start_page + count; ++allocated) {
                    set_bit(allocated);
                    ++used_pages;
                }
                *out_pages = (void *)(uintptr_t)(start_page * PMM_PAGE_SIZE);
                return 0;
            }
        } else {
            free_run = 0;
        }
    }
    return -1;
}

int pmm_free_page(void *phys_addr) {
    size_t page = (size_t)((uintptr_t)phys_addr / PMM_PAGE_SIZE);
    if (page < total_pages && test_bit(page)) {
        clear_bit(page);
        --used_pages;
    }
    return 0;
}

int pmm_free_pages(void *phys_addr, size_t count) {
    uintptr_t base_addr = (uintptr_t)phys_addr;
    for (size_t i = 0; i < count; ++i) {
        if (i > (UINTPTR_MAX - base_addr) / PMM_PAGE_SIZE) {
            break;
        }
        pmm_free_page((void *)(base_addr + i * PMM_PAGE_SIZE));
    }
    return 0;
}

size_t pmm_get_total_pages(void) { return total_pages; }
size_t pmm_get_used_pages(void) { return used_pages; }
size_t pmm_get_free_pages(void) { return total_pages - used_pages; }
size_t pmm_get_total_memory(void) { return total_pages * PMM_PAGE_SIZE; }
size_t pmm_get_used_memory(void) { return used_pages * PMM_PAGE_SIZE; }
size_t pmm_get_free_memory(void) { return (total_pages - used_pages) * PMM_PAGE_SIZE; }
