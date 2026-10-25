#ifndef CELESTEOS_PMM_H
#define CELESTEOS_PMM_H

#include <limine.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define PHYS_PAGE_SIZE 4096 /* 4 KiB pages*/

int pmm_init(struct limine_memmap_response *memmap, uint64_t hhdm_base_offset);

/**
 * Allocate a single page of physical memory.
 */
int pmm_alloc_page(void **page);

/**
 * Allocate multiple contiguous pages of physical memory.
 */
int pmm_alloc_pages(size_t count, void **out_pages);

/**
 * Free a single page of physical memory.
 */
int pmm_free_page(void *page);

/**
 * Free multiple contiguous pages of physical memory.
 */
int pmm_free_pages(void *start_page, size_t count);

size_t pmm_get_total_pages();
size_t pmm_get_used_pages();
size_t pmm_get_free_pages();
size_t pmm_get_total_memory();
size_t pmm_get_used_memory();
size_t pmm_get_free_memory();

#endif // CELESTEOS_PMM_H