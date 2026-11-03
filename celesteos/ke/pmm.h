#ifndef CELESTEOS_PMM_H
#define CELESTEOS_PMM_H

#include <limine.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define PHYS_PAGE_SIZE 4096 /* 4 KiB pages*/

int KeInitPhysMemoryManager(struct limine_memmap_response *memmap, uint64_t hhdm_base_offset);

/**
 * Allocate a single page of physical memory.
 */
int KeAllocPhysPage(void **page);

/**
 * Allocate multiple contiguous pages of physical memory.
 */
int KeAllocPhysPages(size_t count, void **out_pages);

/**
 * Free a single page of physical memory.
 */
int KeFreePhysPage(void *page);

/**
 * Free multiple contiguous pages of physical memory.
 */
int KeFreePhysPages(void *start_page, size_t count);

size_t KeGetTotalPages();
size_t KeGetUsedPages();
size_t KeGetFreePages();
size_t KeGetTotalMemory();
size_t KeGetUsedMemory();
size_t KeGetFreeMemory();

#endif // CELESTEOS_PMM_H