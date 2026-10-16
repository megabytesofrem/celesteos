#include "kmalloc.h"

#include <stdint.h>

#include "pmm.h"

/* Start address of the heap */
static BlockHeader* heap_start;

/* Current virtual address for the heap */
static uintptr_t heap_curr_virt = 0xFFFF900000000000ULL;

bool heap_initialize(vm_map_t* kernel_vm_map) {
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

BlockHeader* heap_expand(size_t min_size) {
    // Calculate how many pages are needed to satisfy the (size + header)
    size_t total_size = min_size + sizeof(BlockHeader);
    size_t pages_needed = (total_size + VMM_PAGE_SIZE - 1) / VMM_PAGE_SIZE;
    size_t bytes_needed = pages_needed * VMM_PAGE_SIZE;

    uintptr_t new_heap_virt = heap_curr_virt;

    // Allocate each page sequentially from vm_map
    for (size_t i = 0; i < pages_needed; i++) {
        void* page = vm_map_alloc(kernel_vm_map, new_heap_virt, VM_PROT_READ | VM_PROT_WRITE);
        if (!page) {
            // If any page allocation fails, return NULL to indicate failure.
            return NULL;
        }

        // Move to the next virtual address for the heap.
        new_heap_virt += VMM_PAGE_SIZE;
    }

    // Initialize a new free block header at the start of the newly allocated heap space.
    BlockHeader* new_block = (BlockHeader*)new_heap_virt;
    new_block->size = bytes_needed - sizeof(BlockHeader);
    new_block->is_free = true;
    new_block->next = NULL;

    // Append the new block to the end of the current heap list.
    BlockHeader* current = heap_start;
    while (current->next != NULL) {
        current = current->next;
    }
    current->next = new_block;

    return new_block;
}

void inner_alloc(BlockHeader* block_hdr, size_t size) {
    /* Split block if larger than needed */
    if (block_hdr->size >= size + sizeof(BlockHeader) + 16) {
        BlockHeader* new_block = (BlockHeader*)((uintptr_t)block_hdr + sizeof(BlockHeader) + size);
        new_block->size = block_hdr->size - size - sizeof(BlockHeader);
        new_block->is_free = true;
        new_block->next = block_hdr->next;

        block_hdr->size = size;
        block_hdr->next = new_block;
    }
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
                inner_alloc(current, size);
            }

            // Mark the current block as allocated and return its address.
            current->is_free = false;
            return (void*)((uintptr_t)current + sizeof(BlockHeader));
        }
        current = current->next;
    }

    // No free block found, try to expand the heap.
    BlockHeader* expanded_block = heap_expand(size);
    if (!expanded_block) {
        // We are truly out of memory.
        return NULL;
    }

    // Allocate memory from the newly expanded heap block.
    inner_alloc(expanded_block, size);
    expanded_block->is_free = false;

    return (void*)((uintptr_t)expanded_block + sizeof(BlockHeader));
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
