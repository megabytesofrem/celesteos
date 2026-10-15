#include "heap_alloc.h"

#include "pmm.h"
#include "vmm.h"

namespace memory {

struct BlockHeader {
    size_t size;
    bool is_free;

    // Pointer to the next block in the heap
    BlockHeader* next;
};

static BlockHeader* heap_start = nullptr;

// Arbitrary starting virtual address for the heap
static uintptr_t heap_curr_virt = 0xFFFF900000000000;

bool initialize_heap(PageTable* kernel_pml4) {
    PhysMemoryManager& pmm = PhysMemoryManager::instance();
    VirtualMemoryManager& vmm = VirtualMemoryManager::instance();

    // Grab initial page for the heap
    uintptr_t phys_page = reinterpret_cast<uintptr_t>(pmm.alloc_page());
    if (phys_page == 0) return false;

    // Map the initial heap page into the virtual address space
    if (!vmm.map(kernel_pml4, heap_curr_virt, phys_page,
                 static_cast<uint64_t>(PageFlags::Present) |
                 static_cast<uint64_t>(PageFlags::ReadWrite))) {
        pmm.free_page(reinterpret_cast<void*>(phys_page));
        return false;
    }

    heap_start = reinterpret_cast<BlockHeader*>(heap_curr_virt);
    heap_start->size = 4096 - sizeof(BlockHeader); /* Initial block size minus header */
    heap_start->is_free = true;
    heap_start->next = nullptr;
    return true;
}

void* kmalloc(size_t size) {
    if (size == 0) return nullptr;

    size = (size + 7) & ~static_cast<size_t>(7); /* align to 8 bytes */

    // Loop
    BlockHeader* curr = heap_start;
    while (curr) {
        if (curr->is_free && curr->size >= size) {
            // Split block if there is room for another header + minimum payload
            if (curr->size >= size + sizeof(BlockHeader) + 16) {
                auto* new_block = reinterpret_cast<BlockHeader*>(
                    reinterpret_cast<uintptr_t>(curr) + sizeof(BlockHeader) + size);
                new_block->size = curr->size - size - sizeof(BlockHeader);
                new_block->is_free = true;
                new_block->next = curr->next;

                curr->size = size;
                curr->next = new_block;
            }

            // Mark as no longer free
            curr->is_free = false;
            return reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(curr) + sizeof(BlockHeader));
        }

        // Move to the next block in the heap
        curr = curr->next;
    }

    // No suitable free block found, return nullptr
    return nullptr;
}

void kfree(void* ptr) {
    if (!ptr) return;

    auto* block = reinterpret_cast<BlockHeader*>(reinterpret_cast<uintptr_t>(ptr) - sizeof(BlockHeader));
    block->is_free = true;

    // Merge adjacent free blocks
    BlockHeader* curr = heap_start;
    while (curr && curr->next) {
        if (curr->is_free && curr->next->is_free) {
            // Merge the current block with the next free block
            curr->size += sizeof(BlockHeader) + curr->next->size;
            curr->next = curr->next->next;
        } else {
            curr = curr->next;
        }
    }
}

}  // namespace memory