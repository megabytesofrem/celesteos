#include "vm_map.h"

#include <kernel/memory/kmalloc.h>
#include <kernel/memory/pmm.h>
#include <libkernel.h>

vm_map_t* kernel_vm_map;

static vm_map_t bootstrap_kernel_map;
static vm_map_entry_t bootstrap_kernel_entries[VM_MAP_MAX_ENTRIES];

bool vm_map_initialize_kernel(page_table_t* page_table) {
    if (!page_table || kernel_vm_map) {
        return false;
    }

    spinlock_init(&bootstrap_kernel_map.lock);
    bootstrap_kernel_map.page_table = page_table;
    bootstrap_kernel_map.entries = bootstrap_kernel_entries;
    bootstrap_kernel_map.entry_count = 0;
    bootstrap_kernel_map.total_size = 0;
    kernel_vm_map = &bootstrap_kernel_map;
    return true;
}

vm_map_t* vm_map_create(page_table_t* pml4) {
    vm_map_t* map = (vm_map_t*)kmalloc(sizeof(vm_map_t));
    if (!map) {
        return NULL;
    }

    // Initialize the vm_map structure fields
    spinlock_init(&map->lock);
    map->page_table = pml4;
    map->entry_count = 0;
    map->total_size = 0;

    // Preallocate VM_MAP_MAX_ENTRIES many entries
    map->entries = (vm_map_entry_t*)kmalloc(sizeof(vm_map_entry_t) * VM_MAP_MAX_ENTRIES);
    if (!map->entries) {
        kfree(map); /* Clean up map on failure */
        return NULL;
    }

    return map;
}

void vm_map_destroy(vm_map_t* map) {
    if (!map || map == kernel_vm_map) {
        return;
    }

    // Free the entries array if it was allocated
    if (map->entries) {
        kfree(map->entries);
    }

    // Free the vm_map structure itself
    kfree(map);
}

bool vm_map_insert(vm_map_t* map, vm_offset_t start, vm_offset_t end, vm_prot_t prot) {
    if (!map || !map->entries || start >= end) {
        return false;
    }

    for (size_t i = 0; i < map->entry_count; ++i) {
        if (start < map->entries[i].end && end > map->entries[i].start) {
            return false;
        }
    }

    // Check if there is space for a new entry
    if (map->entry_count >= VM_MAP_MAX_ENTRIES) {
        return false;
    }

    // Insert a new entry at the end of the entries
    vm_map_entry_t* entry = &map->entries[map->entry_count];
    entry->start = start;
    entry->end = end;
    entry->prot = prot;

    // Update the entry count and total size of the vm_map
    map->entry_count++;

    // Increase the total size by the size of the new entry, so the size of
    // the entry that was just added is correctly reflected in the total size
    map->total_size += (end - start);

    return true;
}

bool vm_map_remove(vm_map_t* map, vm_offset_t start, vm_offset_t end) {
    if (!map) {
        return false;
    }

    for (size_t i = 0; i < map->entry_count; i++) {
        vm_map_entry_t* entry = &map->entries[i];
        if (entry->start == start && entry->end == end) {
            // Shift all subsequent entries down by one
            for (size_t j = i; j < map->entry_count - 1; j++) {
                map->entries[j] = map->entries[j + 1];
            }

            // Update the entry count and total size of the vm_map
            map->entry_count--;

            // Decrease the total size by (end - start), so the size of
            // the entry that was removed is correctly reflected in the total size
            map->total_size -= (end - start);
            return true;
        }
    }
    return false;
}

bool vm_map_protect(vm_map_t* map, vm_offset_t start, vm_offset_t end, vm_prot_t prot) {
    if (!map) {
        return false;
    }

    // Iterate through the entries and update the protection flags for the specified range
    for (size_t i = 0; i < map->entry_count; i++) {
        vm_map_entry_t* entry = &map->entries[i];
        if (entry->start >= start && entry->end <= end) {
            entry->prot = prot;
        }
    }

    return true;
}

void* vm_map_alloc(vm_map_t* map, uint64_t virt_addr, vm_prot_t prot) {
    if (!map || !map->page_table) {
        return NULL;
    }

    uint64_t page_aligned_addr = virt_addr & ~((uint64_t)VMM_PAGE_SIZE - 1);
    if (page_aligned_addr > UINT64_MAX - VMM_PAGE_SIZE) {
        return NULL;
    }

    // Insert the new page into the vm_map
    if (!vm_map_insert(map, page_aligned_addr, page_aligned_addr + VMM_PAGE_SIZE, prot)) {
        return NULL;
    }

    // Grab a physical page from the PMM
    void* phys_page = pmm_alloc_page();
    if (!phys_page) {
        vm_map_remove(map, page_aligned_addr, page_aligned_addr + VMM_PAGE_SIZE);
        return NULL;
    }

    // Convert vm_prot_t flags to page table entry flags
    uint64_t pte_flags = PAGE_PRESENT;
    if (prot & VM_PROT_WRITE)
        pte_flags |= PAGE_READ_WRITE;
    if (!(prot & VM_PROT_WRITE))
        pte_flags |= PAGE_NO_EXECUTE;
    if (map != kernel_vm_map)
        pte_flags |= PAGE_USER;

    // Map the virtual address to the physical page in the page table
    if (!vmm_map(map->page_table, page_aligned_addr, (uint64_t)phys_page, pte_flags)) {
        // Failed to map the page, clean up
        vm_map_remove(map, page_aligned_addr, page_aligned_addr + VMM_PAGE_SIZE);
        pmm_free_page(phys_page);
        return NULL;
    }

    // Return the page-aligned virtual address of the newly allocated memory region
    return (void*)(uintptr_t)page_aligned_addr;
}

bool vm_map_free(vm_map_t* map, uint64_t virt_addr) {
    if (!map || !map->page_table || (virt_addr & (VMM_PAGE_SIZE - 1)) ||
        virt_addr > UINT64_MAX - VMM_PAGE_SIZE) {
        klog(KLOG_ERROR, "vm-map: Invalid page release request.\n");
        return false;
    }

    vm_map_entry_t* entry = vm_map_lookup(map, virt_addr);
    uintptr_t physical = vmm_get_physical(map->page_table, virt_addr);
    if (!entry || entry->start != virt_addr ||
        entry->end != virt_addr + VMM_PAGE_SIZE || !physical) {
        klog(KLOG_ERROR, "vm-map: Page release requires an allocated single-page region.\n");
        return false;
    }

    vmm_unmap(map->page_table, virt_addr);
    vm_map_remove(map, virt_addr, virt_addr + VMM_PAGE_SIZE);
    pmm_free_page((void*)physical);
    return true;
}

vm_map_entry_t* vm_map_lookup(vm_map_t* map, uint64_t addr) {
    if (!map) {
        return NULL;
    }

    // Loop through all entries in vm_map
    for (size_t i = 0; i < map->entry_count; i++) {
        // Pull the current entry from the entries array (entries[i])
        vm_map_entry_t* entry = &map->entries[i];

        // Check if the address falls within the current entry's range
        if (addr >= entry->start && addr < entry->end) {
            // BINGO. Return the entry
            return entry;
        }
    }

    // No entry found
    return NULL;
}