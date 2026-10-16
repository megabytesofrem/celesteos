#ifndef CELESTEOS_KMALLOC_H
#define CELESTEOS_KMALLOC_H

#include <stdbool.h>
#include <stddef.h>

#include "vmm.h"
#include "vm_map.h"

/**
 * Initialize the heap with a given kernel virtual memory map.
 */
bool initialize_heap(vm_map_t* kernel_vm_map);

/**
 * Kernel malloc function.
 */
void* kmalloc(size_t size);

/**
 * Kernel free function.
 */
void kfree(void* ptr);

#endif  // CELESTEOS_KMALLOC_H
