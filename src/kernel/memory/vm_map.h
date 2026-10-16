#ifndef CELESTEOS_VM_MAP_H
#define CELESTEOS_VM_MAP_H

#include <stdint.h>

#include <kernel/memory/pmap.h>
#include <kernel/spinlock.h>
#include <mach/kern_return.h>

typedef uint32_t vm_prot_t;
typedef uintptr_t vm_offset_t;

#define VM_MAP_MAX_ENTRIES 1024

#define VM_PROT_READ  (1 << 0) /* READ permission */
#define VM_PROT_WRITE (1 << 1) /* WRITE permission */
#define VM_PROT_EXEC  (1 << 2) /* EXECUTE permission */

#define VM_PROT_READ_EXEC (VM_PROT_READ | VM_PROT_EXEC)                 /* READ and EXECUTE permissions */
#define VM_PROT_ALL       (VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXEC) /* ALL permissions */

typedef struct vm_map_entry {
    struct vm_map_entry* next;
    struct vm_map_entry* prev;

    vm_offset_t start; /* start address of the memory region */
    vm_offset_t end;   /* end address of the memory region */
    vm_prot_t prot;    /* protection flags for the memory region */
} vm_map_entry_t;

typedef struct vm_map {
    spinlock_t lock;          /* spinlock to protect the virtual memory map */
    page_table_t* page_table; /* pointer to the page table for this virtual memory map */

    vm_map_entry_t* entries; /* pointer to the list of virtual memory map entries */
    size_t entry_count;      /* number of entries in the virtual memory map */
    size_t total_size;       /* total size of all memory regions in the virtual memory map */
} vm_map_t;

/* Global pointer to the higher-half kernel address space map */
extern vm_map_t* kernel_vm_map;

/**
 * Initialize the kernel map using static storage before the heap is available.
 */
kern_return_t vm_map_initialize_kernel(page_table_t* page_table);

/**
 * Create and initialize a new virtual memory map.
 */
kern_return_t vm_map_create(page_table_t* page_table, vm_map_t** out_map);

/**
 * Destroy a virtual memory map and free its resources.
 */
kern_return_t vm_map_destroy(vm_map_t* map);

/**
 * Insert a new memory region into the virtual memory map.
 */
kern_return_t vm_map_insert(vm_map_t* map, vm_offset_t start, vm_offset_t end, vm_prot_t prot);

/**
 * Remove a memory region from the virtual memory map.
 */
kern_return_t vm_map_remove(vm_map_t* map, vm_offset_t start, vm_offset_t end);

/**
 * Allocate a new memory region in the virtual memory map.
 */
kern_return_t vm_map_alloc(vm_map_t* map, uint64_t virt_addr, vm_prot_t prot, void** out_virt_addr);

/**
 * Unmap and free one page allocated by vm_map_alloc.
 * The address must be page-aligned. Page-table structures are retained.
 */
kern_return_t vm_map_free(vm_map_t* map, uint64_t virt_addr);

/**
 * Change the protection flags of a memory region in the virtual memory map.
 */
kern_return_t vm_map_protect(vm_map_t* map, vm_offset_t start, vm_offset_t end, vm_prot_t prot);

/**
 * Lookup the memory region containing the specified virtual address.
 * 
 * Returns NULL if lookup fails (i.e., no memory region contains the specified address).
 */
vm_map_entry_t* vm_map_lookup(vm_map_t* map, uint64_t addr);

#endif  // CELESTEOS_VM_MAP_H