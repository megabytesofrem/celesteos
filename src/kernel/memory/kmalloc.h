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
kern_return_t heap_initialize(vm_map_t* kernel_vm_map);

/**
 * Expand the initialized heap with virtually contiguous pages.
 * Returns a free block with at least min_size bytes, or NULL on failure.
 * Failed growth leaves existing heap blocks and the heap end unchanged.
 * Calls to heap operations must be serialized by the caller.
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
