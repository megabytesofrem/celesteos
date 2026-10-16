#include "kmalloc.h"

#include <stdint.h>

#include "pmm.h"

typedef struct BlockHeader {
    size_t size;
    bool is_free;
    struct BlockHeader* next;
} BlockHeader;

static BlockHeader* heap_start;
static uintptr_t heap_curr_virt = 0xFFFF900000000000ULL;

bool initialize_heap(vm_map_t* kernel_vm_map) {
    if (!kernel_vm_map) {
        return false;
    }

    // Ask the kernels vm_map structure to allocate and back the initial heap page.
    void* initial_heap_page = vm_map_alloc(kernel_vm_map, heap_curr_virt, VM_PROT_READ | VM_PROT_WRITE);
    if (!initial_heap_page) {
        return false;
    }

    // Initialize the heap start with the allocated initial heap page.
    heap_start = (BlockHeader*)heap_curr_virt;
    heap_start->size = VMM_PAGE_SIZE - sizeof(BlockHeader); /* initial block size - header size */
    heap_start->is_free = true;
    heap_start->next = NULL;
    return true;
}

void* kmalloc(size_t size) {
    if (size == 0 ||
        size > SIZE_MAX - 7 - sizeof(BlockHeader) - 16) {
        return NULL;
    }
    size = (size + 7) & ~(size_t)7;

    BlockHeader* current = heap_start;
    while (current != NULL) {
        if (current->is_free && current->size >= size) {
            // Check if the current block can be split into two blocks.
            if (current->size >= size + sizeof(BlockHeader) + 16) {
                // Split the current block into two blocks if there is room
                BlockHeader* new_block = (BlockHeader*)((uintptr_t)current + sizeof(BlockHeader) + size);
                new_block->size =
                    current->size - size - sizeof(BlockHeader);
                new_block->is_free = true;
                new_block->next = current->next;

                current->size = size;
                current->next = new_block;
            }

            // Mark the current block as allocated and return its address.
            current->is_free = false;
            return (void*)((uintptr_t)current + sizeof(BlockHeader));
        }
        current = current->next;
    }

    // No suitable free block was found, return NULL.
    return NULL;
}

void kfree(void* ptr) {
    if (ptr == NULL) {
        return;
    }

    BlockHeader* block = (BlockHeader*)((uintptr_t)ptr - sizeof(BlockHeader));

    // Mark the block as free so it can be reused.
    block->is_free = true;

    // Loop through the heap and merge adjacent free blocks.
    BlockHeader* current = heap_start;
    while (current != NULL && current->next != NULL) {
        if (current->is_free && current->next->is_free) {
            current->size += sizeof(BlockHeader) + current->next->size;
            current->next = current->next->next;
        } else {
            current = current->next;
        }
    }
}
