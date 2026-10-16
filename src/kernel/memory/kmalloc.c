#include "kmalloc.h"

#include <libkernel.h>
#include <stdint.h>

/* Start address of the heap (linked list of memory blocks) */
static BlockHeader* heap_start;
static vm_map_t* heap_map;

/* First unmapped heap address; reserve one PML4 slot for heap growth. */
static uintptr_t heap_curr_virt = 0xFFFF900000000000ULL;

/* Maximum heap size limit (the highest virtual address the heap can grow to) */
static const uintptr_t heap_limit = 0xFFFF908000000000ULL;

int heap_initialize(vm_map_t* kernel_vm_map) {
    if (!kernel_vm_map || heap_start) {
        return -1;
    }

    // Ask the kernels vm_map structure to allocate and back the initial heap page.
    void* initial_heap_page;
    if (vm_map_alloc(kernel_vm_map, heap_curr_virt, VM_PROT_READ | VM_PROT_WRITE, &initial_heap_page) != 0) {
        return -1;
    }

    // Initialize the heap start with the allocated initial heap page.
    heap_start = (BlockHeader*)heap_curr_virt;
    heap_start->size = PMAP_PAGE_SIZE - sizeof(BlockHeader); /* initial block size - header size */
    heap_start->is_free = true;
    heap_start->next = NULL;
    heap_map = kernel_vm_map;
    heap_curr_virt += PMAP_PAGE_SIZE;
    return 0;
}

BlockHeader* heap_expand(size_t min_size) {
    if (!heap_start || !heap_map || min_size == 0 ||
        min_size > SIZE_MAX - 7 - sizeof(BlockHeader) - (PMAP_PAGE_SIZE - 1)) {
        klog(KLOG_ERROR, "heap: Cannot expand an uninitialized heap or satisfy an invalid size.\n");
        return NULL;
    }
    min_size = (min_size + 7) & ~(size_t)7;

    // Find the last block in the heap linked list, to expand after it.
    BlockHeader* tail = heap_start;
    while (tail->next) {
        tail = tail->next;
    }

    // Calculate the total size needed for the new block, including the header.
    size_t total_size = min_size + sizeof(BlockHeader);
    if (tail->is_free) {
        if (tail->size >= min_size) {
            return tail;
        }
        total_size = min_size - tail->size;
    }

    // Calculate the number of pages needed to satisfy the total size.
    size_t pages_needed = (total_size + PMAP_PAGE_SIZE - 1) / PMAP_PAGE_SIZE;

    // Calculate the total number of bytes needed for the new pages.
    size_t bytes_needed = pages_needed * PMAP_PAGE_SIZE;

    // Safety check: Ensure that the heap has enough space and the VM map has enough entries for the new pages.
    if (heap_curr_virt >= heap_limit ||
        bytes_needed > heap_limit - heap_curr_virt ||
        pages_needed > VM_MAP_MAX_ENTRIES - heap_map->entry_count) {
        klog(KLOG_ERROR, "heap: Heap address range or VM map capacity exhausted.\n");
        return NULL;
    }

    // Allocate each page sequentially from vm_map
    for (size_t i = 0; i < pages_needed; i++) {
        uintptr_t address = heap_curr_virt + i * PMAP_PAGE_SIZE;
        void* page;

        if (vm_map_alloc(heap_map, address, VM_PROT_READ | VM_PROT_WRITE, &page) != 0) {
            // Roll back on allocation failure
            bool rolled_back = true;
            while (i > 0) {
                --i;
                if (vm_map_free(heap_map, heap_curr_virt + i * PMAP_PAGE_SIZE) != 0) {
                    rolled_back = false;
                }
            }
            if (rolled_back) {
                klog(KLOG_ERROR, "heap: Failed to map expansion pages; growth rolled back.\n");
            } else {
                klog(KLOG_ERROR, "heap: Failed to roll back all expansion pages.\n");
            }
            return NULL;
        }
    }

    BlockHeader* new_block;

    // If the last block is free, try to extend it; otherwise, create a new block.
    if (tail->is_free) {
        tail->size += bytes_needed;
        new_block = tail;
    } else {
        // Allocate a new block at the current heap virtual address.
        new_block = (BlockHeader*)heap_curr_virt;
        new_block->size = bytes_needed - sizeof(BlockHeader);
        new_block->is_free = true;
        new_block->next = NULL;
        tail->next = new_block;
    }

    heap_curr_virt += bytes_needed;
    return new_block;
}

void inner_alloc(BlockHeader* block_hdr, size_t size) {
    // Split the block into an allocated part and a free part if it's larger than needed.
    BlockHeader* new_block = (BlockHeader*)((uintptr_t)block_hdr + sizeof(BlockHeader) + size);

    size_t block_offset_sz = block_hdr->size - size - sizeof(BlockHeader);
    new_block->size = block_offset_sz; /* remaining size for the new free block */
    new_block->is_free = true;         /* mark the new block as free block */
    new_block->next = block_hdr->next; /* keep the next pointer of the original block */

    // Update the original block to reflect the new size after splitting.
    block_hdr->size = size;
    block_hdr->next = new_block;
}

void* kmalloc(size_t size) {
    size_t max_size = SIZE_MAX - 7 - sizeof(BlockHeader) - 16;
    size_t aligned_size;

    if (size == 0 || size > max_size) {
        return NULL;
    }

    // Align the number of bytes (the size) to an 8-byte boundary
    aligned_size = (size + 7) & ~(size_t)7;

    size = aligned_size;

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

    // Check if we have run out of memory for allocations
    if (!expanded_block) {
        return NULL;
    }

    // Check if there is enough left over space for a new free block after allocation.
    if (expanded_block->size >= size + sizeof(BlockHeader) + 16) {
        // Split the expanded block into an allocated part and a free part
        inner_alloc(expanded_block, size);
    }

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
