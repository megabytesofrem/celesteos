#ifndef CELESTEOS_PMM_H
#define CELESTEOS_PMM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "../../limine.h"

#define PMM_PAGE_SIZE 0x1000 /* 4KiB */

/**
 * Initialize the physical memory manager with the given memory map.
 */
bool pmm_initialize(struct limine_memmap_response* memmap,
                    uint64_t hhdm_base_offset);

/**
 * Allocate a single page of physical memory.
 */
void* pmm_alloc_page(void);

/**
 * Allocate multiple contiguous pages of physical memory.
 */
void* pmm_alloc_pages(size_t count);

/**
 * Free a single page of physical memory.
 */
void pmm_free_page(void* page);

/**
 * Free multiple contiguous pages of physical memory.
 */
void pmm_free_pages(void* pages, size_t count);

size_t pmm_get_total_pages(void);
size_t pmm_get_used_pages(void);
size_t pmm_get_free_pages(void);
size_t pmm_get_total_memory(void);
size_t pmm_get_used_memory(void);
size_t pmm_get_free_memory(void);

#endif
