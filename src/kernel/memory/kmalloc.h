#ifndef CELESTEOS_KMALLOC_H
#define CELESTEOS_KMALLOC_H

#include <stdbool.h>
#include <stddef.h>

#include "vmm.h"

/**
 * Initialize the heap with a given kernel page table.
 */
bool initialize_heap(page_table_t* kernel_pml4);

/**
 * Kernel malloc function.
 */
void* kmalloc(size_t size);

/**
 * Kernel free function.
 */
void kfree(void* ptr);

#endif  // CELESTEOS_KMALLOC_H
