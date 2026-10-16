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

bool initialize_heap(page_table_t* kernel_pml4) {
    // Grab initial page for the heap
    void* phys_page = pmm_alloc_page();
    if (phys_page == NULL) {
        return false;
    }

    // Map the initial heap page into the virtual address space
    if (!vmm_map(kernel_pml4,
                 heap_curr_virt,
                 (uint64_t)(uintptr_t)phys_page,
                 PAGE_PRESENT | PAGE_READ_WRITE)) {
        pmm_free_page(phys_page);
        return false;
    }

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
