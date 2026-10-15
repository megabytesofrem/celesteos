#include "pmm.h"

namespace memory {

PhysMemoryManager& PhysMemoryManager::instance() {
    static PhysMemoryManager instance;
    return instance;
}

void PhysMemoryManager::set_bit(size_t page) {
    bitmap[page / 8] |= (1 << (page % 8));
}

void PhysMemoryManager::clear_bit(size_t page) {
    bitmap[page / 8] &= ~(1 << (page % 8));
}

bool PhysMemoryManager::test_bit(size_t page) const {
    return (bitmap[page / 8] & (1 << (page % 8))) != 0;
}

/**
 * Initialize the physical memory manager.
 *
 * @param memmap: Pointer to Limine's memory map response.
 * @param hh_offset: Higher half offset for physical memory mapping.
 */
void PhysMemoryManager::initialize(limine_memmap_response* memmap, uint64_t hhdm_base_offset) {
    this->higher_half_base = hhdm_base_offset;
    this->used_pages = 0;

    // Calculate highest usable address
    uint64_t highest_address = 0x0;
    for (size_t i = 0; i < memmap->entry_count; i++) {
        limine_memmap_entry* entry = memmap->entries[i];
        if (entry->type == LIMINE_MEMMAP_USABLE) {
            uint64_t end_address = entry->base + entry->length;
            if (end_address > highest_address) {
                highest_address = end_address;
            }
        }
    }

    this->total_pages = highest_address / PHYS_PAGE_SIZE;
    this->bitmap_size = (total_pages + 7) / 8;

    // Align bitmap size up to page boundaries
    uint64_t bitmap_pages = (bitmap_size + PHYS_PAGE_SIZE - 1) / PHYS_PAGE_SIZE;
    uint64_t bitmap_size_aligned = bitmap_pages * PHYS_PAGE_SIZE;

    // Find a usable region large enough for the bitmap
    for (size_t i = 0; i < memmap->entry_count; i++) {
        limine_memmap_entry* entry = memmap->entries[i];
        if (entry->type == LIMINE_MEMMAP_USABLE && entry->length >= bitmap_size_aligned) {
            bitmap = reinterpret_cast<uint8_t*>(entry->base + higher_half_base);
            break;
        }
    }

    // Mark all pages as used initially
    for (size_t i = 0; i < bitmap_size; i++) {
        bitmap[i] = 0xFF;
    }

    // Free usable pages from Limine's memory map
    for (size_t i = 0; i < memmap->entry_count; i++) {
        limine_memmap_entry* entry = memmap->entries[i];
        if (entry->type == LIMINE_MEMMAP_USABLE) {
            uint64_t start_page = entry->base;
            uint64_t end_page = entry->base + entry->length;

            for (uint64_t addr = start_page; addr < end_page; addr += PHYS_PAGE_SIZE) {
                size_t page = addr / PHYS_PAGE_SIZE;
                clear_bit(page);
            }
        }
    }

    // Reserve the physical pages backing the bitmap itself
    uint64_t bitmap_phys = reinterpret_cast<uint64_t>(bitmap) - higher_half_base;
    for (uint64_t addr = bitmap_phys; addr < bitmap_phys + bitmap_size_aligned; addr += PHYS_PAGE_SIZE) {
        set_bit(addr / PHYS_PAGE_SIZE);
        this->used_pages++;
    }

    // Reserve Page 0 (guard against null pointer dereference)
    set_bit(0);
    this->used_pages++;
}

/**
 * Allocate a single physical page of memory.
 *
 * Returns a pointer to the page, or nullptr if out of memory.
 */
void* PhysMemoryManager::alloc_page() {
    for (size_t page = 0; page < total_pages; page++) {
        if (!test_bit(page)) {
            set_bit(page);
            this->used_pages++;

            // Return the physical address of the allocated page
            return reinterpret_cast<void*>(page * PHYS_PAGE_SIZE);
        }
    }
    return nullptr;  // Out of physical memory
}

/**
 * Allocate multiple contiguous physical pages of memory.
 *
 * @param count: The number of contiguous pages to allocate.
 * Returns a pointer to the first allocated page, or nullptr if out of contiguous physical memory.
 */
void* PhysMemoryManager::alloc_pages(size_t count) {
    if (count == 0) return nullptr;
    if (count == 1) return alloc_page();

    size_t free_run = 0;
    size_t start_page = 0;

    for (size_t i = 0; i < total_pages; i++) {
        if (!test_bit(i)) {
            if (free_run == 0) start_page = i;
            free_run++;

            if (free_run == count) {
                // Found a contiguous block of free pages, mark all of them as used
                for (size_t j = start_page; j < start_page + count; j++) {
                    set_bit(j);
                    this->used_pages++;
                }
                return reinterpret_cast<void*>(start_page * PHYS_PAGE_SIZE);
            }
        } else {
            // Reset the free run counter if an already used page is encountered
            free_run = 0;
        }
    }
    return nullptr;  // Out of contiguous physical memory
}

/**
 * Free a single physical page of memory.
 *
 * @param phys_addr: The physical address of the page to free.
 */
void PhysMemoryManager::free_page(void* phys_addr) {
    size_t page = reinterpret_cast<uint64_t>(phys_addr) / PHYS_PAGE_SIZE;
    if (test_bit(page)) {
        clear_bit(page);
        this->used_pages--;
    }
}

/**
 * Free multiple contiguous physical pages of memory.
 *
 * @param phys_addr: The physical address of the first page to free.
 * @param count: The number of contiguous pages to free.
 */
void PhysMemoryManager::free_pages(void* phys_addr, size_t count) {
    size_t base_addr = reinterpret_cast<uint64_t>(phys_addr);
    for (size_t i = 0; i < count; i++) {
        free_page(reinterpret_cast<void*>(base_addr + (i * PHYS_PAGE_SIZE)));
    }
}

}  // namespace memory