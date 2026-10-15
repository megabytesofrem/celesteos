#ifndef CELESTEOS_PMM_H
#define CELESTEOS_PMM_H

#include <cstddef>
#include <cstdint>

#include "../../limine.h"

namespace memory {

class PhysMemoryManager {
   private:
    PhysMemoryManager() = default;

    static constexpr size_t PHYS_PAGE_SIZE = 4096;

    uint8_t* bitmap = nullptr;
    size_t bitmap_size = 0;

    // Number of total and used physical pages
    size_t total_pages = 0;
    size_t used_pages = 0;

    // Base address of the Higher Half Direct Map (HHDM)
    uint64_t higher_half_base = 0;

    void set_bit(size_t page);
    void clear_bit(size_t page);
    bool test_bit(size_t page) const;

   public:
    static PhysMemoryManager& instance();

    // Delete copy constructor and assignment operator to enforce the singleton pattern
    PhysMemoryManager(const PhysMemoryManager&) = delete;
    PhysMemoryManager& operator=(const PhysMemoryManager&) = delete;

    void initialize(limine_memmap_response* memmap, uint64_t hhdm_base_offset);
    void* alloc_page();
    void* alloc_pages(size_t count);
    void free_page(void* page);
    void free_pages(void* pages, size_t count);

    /* Helpers to query memory usage */

    [[nodiscard]] size_t get_total_pages() const { return total_pages; }
    [[nodiscard]] size_t get_used_pages() const { return used_pages; }
    [[nodiscard]] size_t get_free_pages() const { return total_pages - used_pages; }

    [[nodiscard]] size_t get_total_memory() const { return total_pages * PHYS_PAGE_SIZE; }
    [[nodiscard]] size_t get_used_memory() const { return used_pages * PHYS_PAGE_SIZE; }
    [[nodiscard]] size_t get_free_memory() const { return (total_pages - used_pages) * PHYS_PAGE_SIZE; }
};

}  // namespace memory

#endif  // CELESTEOS_PMM_H