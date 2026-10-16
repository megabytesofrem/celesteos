#ifndef CELESTEOS_KMALLOC_H
#define CELESTEOS_KMALLOC_H

#include <stdbool.h>
#include <stddef.h>

#include "vmm.h"
#include "vm_map.h"

typedef struct BlockHeader {
    size_t size;
    bool is_free;
    struct BlockHeader* next;
} BlockHeader;

/**
 * Initialize the heap with a given kernel virtual memory map.
 */
bool heap_initialize(vm_map_t* kernel_vm_map);

/**
 * Expand the heap by allocating additional pages from kernel_vm_map.
 * 
 * @param min_size The minimum size of the new heap block to allocate.
 */
BlockHeader* heap_expand(size_t min_size);

/**
 * Kernel malloc function.
 * 
 * @param size The number of bytes to allocate from the heap.
 */
void* kmalloc(size_t size);

/**
 * Kernel free function.
 *
 * @param ptr The pointer to the memory block to free.
 */
void kfree(void* ptr);

#endif  // CELESTEOS_KMALLOC_H
