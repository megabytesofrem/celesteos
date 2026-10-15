#ifndef CELESTEOS_VMM_H
#define CELESTEOS_VMM_H

#include <cstddef>
#include <cstdint>

namespace memory {

enum PageFlags : uint64_t {
    Present = 1 << 0,       /* Page is present in memory */
    ReadWrite = 1 << 1,     /* Page is read/write (1 = read/write, 0 = read-only)*/
    User = 1 << 2,          /* Page is accessible by user-mode */
    WriteThrough = 1 << 3,  /* Write-through caching enabled */
    CacheDisabled = 1 << 4, /* Caching disabled for this page */
    Accessed = 1 << 5,      /* Set by CPU when the page is accessed */
    Dirty = 1 << 6,         /* Set by CPU when the page is written to */
    HugePage = 1 << 7,      /* 2 MiB or 1 GiB page */
    Global = 1 << 8,        /* Global page (not flushed from TLB on CR3 reload) */
    NoExecute = 1 << 9      /* Page cannot be executed (NX bit) */
};

struct PageTableEntry {
    uint64_t value;

    void set_address(uint64_t phys_addr) {
        // Physical address occupies bits 12-51 of the page table entry
        value = (value & ~0x000FFFFFFFFFF000ULL) | (phys_addr & 0x000FFFFFFFFFF000ULL);
    }

    uint64_t get_address() const {
        return value & 0x000FFFFFFFFFF000ULL;
    }

    void set_flags(uint64_t flags) {
        value |= flags;
    }

    void clear_flags(uint64_t flags) {
        value &= ~flags;
    }

    bool is_present() const {
        return (value & static_cast<uint64_t>(PageFlags::Present)) != 0;
    }

    bool is_user() const {
        return (value & static_cast<uint64_t>(PageFlags::User)) != 0;
    }

    bool is_writable() const {
        return (value & static_cast<uint64_t>(PageFlags::ReadWrite)) != 0;
    }

    bool is_huge_page() const {
        return (value & static_cast<uint64_t>(PageFlags::HugePage)) != 0;
    }
};

/**
 * Page table aligned to 4096 bytes (4 KiB) with 512 entries.
 */
struct alignas(4096) PageTable {
    PageTableEntry entries[512];
};

class VirtualMemoryManager {
   private:
    VirtualMemoryManager() = default;

    static constexpr size_t VIRT_PAGE_SIZE = 4096;

    PageTable* pml4 = nullptr;

    // Physical address of the PML4 table
    uint64_t pml4_phys_addr = 0;

    // Base address of the Higher Half Direct Map (HHDM)
    uint64_t higher_half_base = 0;

    PageTable* get_or_create_next_level(PageTable* current_level, size_t index, uint64_t flags);

   public:
    VirtualMemoryManager(const VirtualMemoryManager&) = delete;
    VirtualMemoryManager& operator=(const VirtualMemoryManager&) = delete;

    static VirtualMemoryManager& instance();

    void initialize(size_t hhdm_base_offset);
    bool map(PageTable* pml4, uint64_t virt, uint64_t phys, uint64_t flags);
    void unmap(PageTable* pml4, uint64_t virt);
    void switch_pml4(PageTable* new_pml4);

    static inline void flush_tlb(uint64_t virt) {
        asm volatile("invlpg (%0)" ::"r"(virt) : "memory");
    }

    /* Helpers to query the current PML4 and its physical address */
    PageTable* get_pml4() const { return pml4; }
    uint64_t get_pml4_physical() const { return pml4_phys_addr; }
};

}  // namespace memory

#endif  // CELESTEOS_VMM_H