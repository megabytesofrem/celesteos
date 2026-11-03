#include "page.h"

#include <error.h>
#include <ke/pmm.h>
#include <string.h>

/* Kernel page table (PML 4) */
static KePageTable *kPml4;

static uint64_t pml4PhysAddr;
static uint64_t higherHalfBase;

static void KeClearPageTable(KePageTable *table) {
    for (size_t i = 0; i < 512; ++i) {
        table->entries[i].value = 0;
    }
}

static KePageTable *KeGetOrCreateLevel(KePageTable *table, size_t index, uint64_t flags) {
    KePageTableEntry *entry = &table->entries[index];
    if (KeIsPtePresent(entry)) {
        uint64_t nextLevelPhys = KeGetPteAddress(entry);
        return (KePageTable *)(uintptr_t)(nextLevelPhys + higherHalfBase);
    }

    void *newFrame;

    if (KeAllocPhysPage(&newFrame) != 0) {
        return NULL;
    }

    uint64_t newFrameVirtual = (uint64_t)(uintptr_t)newFrame;
    uint64_t newFramePhys = newFrameVirtual - higherHalfBase;
    KePageTable *newTable = (KePageTable *)(uintptr_t)newFrameVirtual;
    KeClearPageTable(newTable);

    // Set page table entry for the new table
    KeSetPteAddress(entry, newFramePhys);
    KeSetPteFlags(entry, PAGE_PRESENT | PAGE_READ_WRITE | flags);
    return newTable;
}

int KePageMapInit(uint64_t hhdmBaseOffset) {
    // Get CR3 which contains the physical address of the page table root
    uintptr_t cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));

    higherHalfBase = hhdmBaseOffset;
    pml4PhysAddr = cr3 & 0x000FFFFFFFFFF000ULL;
    kPml4 = (KePageTable *)(uintptr_t)(pml4PhysAddr + higherHalfBase);
    return 0;
}

KePageTable *KeGetPageTable(void) {
    return kPml4;
}

uint64_t KeGetPageTablePhys(void) {
    return pml4PhysAddr;
}

int KeMapPage(KePageTable *table, uint64_t virtAddr, uint64_t physAddr, uint64_t flags) {
    if (!table) {
        return ERR_INVALID_ARG;
    }

    size_t pml4Index = (size_t)((virtAddr >> 39) & 0x1FF);
    size_t pdptIndex = (size_t)((virtAddr >> 30) & 0x1FF);
    size_t pdIndex = (size_t)((virtAddr >> 21) & 0x1FF);
    size_t ptIndex = (size_t)((virtAddr >> 12) & 0x1FF);

    KePageTable *pdpt = KeGetOrCreateLevel(table, pml4Index, flags); // PDPT
    if (!pdpt) {
        return ERR_INVALID_STATE;
    }
    KePageTable *pd = KeGetOrCreateLevel(pdpt, pdptIndex, flags); // PD
    if (!pd) {
        return ERR_INVALID_STATE;
    }
    KePageTable *pt = KeGetOrCreateLevel(pd, pdIndex, flags); // PT
    if (!pt) {
        return ERR_INVALID_STATE;
    }

    // Get the page table entry for the given virtual address
    KePageTableEntry *pageTableEntry = &pt->entries[ptIndex];

    // Set the physical address and flags and mark it as present and read/write
    KeSetPteAddress(pageTableEntry, physAddr);
    KeSetPteFlags(pageTableEntry, PAGE_PRESENT | PAGE_READ_WRITE | flags);
    return 0;
}

int KeUnmapPage(KePageTable *table, uint64_t virtAddr) {
    if (!table) {
        return ERR_INVALID_ARG;
    }
    size_t pml4Index = (size_t)((virtAddr >> 39) & 0x1FF);
    size_t pdptIndex = (size_t)((virtAddr >> 30) & 0x1FF);
    size_t pdIndex = (size_t)((virtAddr >> 21) & 0x1FF);
    size_t ptIndex = (size_t)((virtAddr >> 12) & 0x1FF);

    KePageTableEntry *entry = &table->entries[pml4Index];
    if (!KeIsPtePresent(entry)) {
        return ERR_INVALID_ARG;
    }
    KePageTable *pdpt = (KePageTable *)(uintptr_t)(KeGetPteAddress(entry) + higherHalfBase);

    entry = &pdpt->entries[pdptIndex];
    if (!KeIsPtePresent(entry)) {
        return ERR_INVALID_ARG;
    }
    KePageTable *pd = (KePageTable *)(uintptr_t)(KeGetPteAddress(entry) + higherHalfBase);

    entry = &pd->entries[pdIndex];
    if (!KeIsPtePresent(entry)) {
        return ERR_INVALID_ARG;
    }
    KePageTable *pt = (KePageTable *)(uintptr_t)(KeGetPteAddress(entry) + higherHalfBase);

    entry = &pt->entries[ptIndex];
    if (!KeIsPtePresent(entry)) {
        return ERR_INVALID_ARG;
    }
    entry->value = 0;

    // Invalidate the TLB for the unmapped virtual address.
    KePageMapInvalidate(virtAddr);
    return 0;
}

KePageTable *KeCreateUserPageTable(uint64_t hhdmBase) {
    if (hhdmBase != higherHalfBase) {
        return NULL;
    }

    void *physPage;
    if (KeAllocPhysPage(&physPage) != 0) {
        return NULL;
    }

    // Grab the higher-half direct mapping address of the newly allocated PML4 page.
    KePageTable *userPml4 = (KePageTable *)physPage;

    // Zero out lower half of the PML4 (indices 0 to 255), reserved for user-space
    memset(&userPml4->entries, 0, sizeof(KePageTableEntry) * 256);

    // Copy the higher-half kernel mappings from the current PML4 to the new user PML4.
    KePageTable *kernelPml4 = KeGetPageTable();

    memcpy(&userPml4->entries[256], &kernelPml4->entries[256], sizeof(KePageTableEntry) * 256);

    // Return the newly allocated user PML4.
    return userPml4;
}

int KePageMapSwitchTo(KePageTable *newPml4) {
    uintptr_t pml4Virtual = (uintptr_t)newPml4;
    if (newPml4 == NULL || pml4Virtual < higherHalfBase) {
        return ERR_INVALID_ARG;
    }

    uint64_t pml4Physical = pml4Virtual - higherHalfBase;
    if ((pml4Physical & (PMAP_PAGE_SIZE - 1)) != 0) {
        return ERR_INVALID_ARG;
    }

    __asm__ volatile("mov %0, %%cr3" : : "r"(pml4Physical) : "memory");
    return 0;
}

int KePageMapInvalidate(uint64_t virtAddr) {
    __asm__ volatile("invlpg (%0)" : : "r"(virtAddr) : "memory");
    return 0;
}

uintptr_t KeTranslatePhysAddress(KePageTable *pml4, uint64_t virtAddr) {
    if (!pml4) {
        return 0;
    }

    // PML4: shifted by 39 bits to get the index
    // PDPT: shifted by 30 bits to get the index
    // PD: shifted by 21 bits to get the index
    // PT: shifted by 12 bits to get the index
    size_t pml4Index = (size_t)((virtAddr >> 39) & 0x1FF);
    size_t pdptIndex = (size_t)((virtAddr >> 30) & 0x1FF);
    size_t pdIndex = (size_t)((virtAddr >> 21) & 0x1FF);
    size_t ptIndex = (size_t)((virtAddr >> 12) & 0x1FF);

    KePageTableEntry *pml4Entry = &pml4->entries[pml4Index];
    if (!KeIsPtePresent(pml4Entry)) {
        return 0;
    }
    KePageTable *pdpt = (KePageTable *)(uintptr_t)(KeGetPteAddress(pml4Entry) + higherHalfBase);
    KePageTableEntry *pdptEntry = &pdpt->entries[pdptIndex];
    if (!KeIsPtePresent(pdptEntry)) {
        return 0;
    }
    KePageTable *pd = (KePageTable *)(uintptr_t)(KeGetPteAddress(pdptEntry) + higherHalfBase);
    KePageTableEntry *pdEntry = &pd->entries[pdIndex];
    if (!KeIsPtePresent(pdEntry)) {
        return 0;
    }
    KePageTable *pt = (KePageTable *)(uintptr_t)(KeGetPteAddress(pdEntry) + higherHalfBase);
    KePageTableEntry *ptEntry = &pt->entries[ptIndex];
    if (!KeIsPtePresent(ptEntry)) {
        return 0;
    }
    return KeGetPteAddress(ptEntry);
}

int KeCopyPageFromUser(void *destination, uintptr_t source, size_t size) {
    // Validate the source address and destination buffer before copying from user space.

    // Define the user-space address limit (47-bit address space) and check if the destination
    // address and size are within this limit and the source buffer is valid.
    const uintptr_t userLimit = UINT64_C(1) << 47;
    if (source >= userLimit || size > userLimit - source ||
        (size != 0 && destination == NULL)) {
        return ERR_INVALID_ARG;
    }

    // Save the current CR3 and get the root page table
    uintptr_t activeCr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(activeCr3));

    // The root page table is obtained from the current CR3 register and mapped into the higher half
    KePageTable *root =
        (KePageTable *)((activeCr3 & UINT64_C(0x000FFFFFFFFFF000)) + higherHalfBase);
    unsigned char *output = destination;

    while (size != 0) {
        KePageTable *table = root;
        uint64_t physical = 0;

        // Traverse the page table hierarchy to find the physical address corresponding to the
        // user-space source address
        for (unsigned int shift = 39;; shift -= 9) {
            // Shift indicates the current level in the page table hierarchy (39 for PML4, 30 for
            // PDPT, 21 for PD, 12 for PT)

            // Get the page table entry for the current level
            KePageTableEntry *entry = &table->entries[(source >> shift) & 0x1FF];
            if (!KeIsPtePresent(entry) || !KeIsUserPage(entry) || !KeIsReadWritePage(entry)) {
                return ERR_INVALID_ARG;
            }

            // If we reach the last level (PT) or encounter a huge page, calculate the physical
            // address
            if (shift == 12 || ((shift == 30 || shift == 21) && KeIsHugePage(entry))) {
                uintptr_t offsetMask = (UINT64_C(1) << shift) - 1;
                physical = (KeGetPteAddress(entry) & ~offsetMask) | (source & offsetMask);
                break;
            }
            if (shift == 39 && KeIsHugePage(entry)) {
                return ERR_INVALID_ARG;
            }
            table = (KePageTable *)(KeGetPteAddress(entry) + higherHalfBase);
        }

        // Calculate the size of the chunk to copy, ensuring it does not cross a page boundary
        size_t chunk = PMAP_PAGE_SIZE - (source & (PMAP_PAGE_SIZE - 1));
        if (chunk > size) {
            chunk = size;
        }

        // Copy the chunk from the physical address to the output buffer
        memcpy(output, (const void *)(physical + higherHalfBase), chunk);
        output += chunk;
        source += chunk;
        size -= chunk;
    }
    return 0;
}

int KePageToUser(uintptr_t destination, const void *source, size_t size) {
    // Validate the destination address and source buffer before copying to user space.

    // Define the user-space address limit (47-bit address space) and check if the destination
    // address and size are within this limit and the source buffer is valid.
    const uintptr_t userLimit = UINT64_C(1) << 47;
    if (destination >= userLimit || size > userLimit - destination ||
        (size != 0 && source == NULL)) {
        return ERR_INVALID_ARG;
    }

    // Save the current CR3 register and get the root page table for the active address space.
    uintptr_t activeCr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(activeCr3));
    KePageTable *root =
        (KePageTable *)((activeCr3 & UINT64_C(0x000FFFFFFFFFF000)) + higherHalfBase);
    const unsigned char *input = source;

    while (size != 0) {
        KePageTable *table = root;
        uint64_t physical = 0;

        // Traverse the page table hierarchy to find the physical address corresponding to the
        // user-space source address
        for (unsigned int shift = 39;; shift -= 9) {
            KePageTableEntry *entry = &table->entries[(destination >> shift) & 0x1FF];
            if (!KeIsPtePresent(entry) || !KeIsUserPage(entry) || !KeIsReadWritePage(entry)) {
                return ERR_INVALID_ARG;
            }
            if (shift == 12 || ((shift == 30 || shift == 21) && KeIsHugePage(entry))) {
                uintptr_t offsetMask = (UINT64_C(1) << shift) - 1;
                physical = (KeGetPteAddress(entry) & ~offsetMask) | (destination & offsetMask);
                break;
            }
            if (shift == 39 && KeIsHugePage(entry)) {
                return ERR_INVALID_ARG;
            }
            table = (KePageTable *)(KeGetPteAddress(entry) + higherHalfBase);
        }

        // Calculate the size of the chunk to copy, ensuring it does not cross a page boundary.
        size_t chunk = PMAP_PAGE_SIZE - (destination & (PMAP_PAGE_SIZE - 1));
        if (chunk > size) {
            chunk = size;
        }

        // Copy the chunk from the input buffer to the physical address in memory.
        memcpy((void *)(physical + higherHalfBase), input, chunk);
        input += chunk;
        destination += chunk;
        size -= chunk;
    }
    return 0;
}