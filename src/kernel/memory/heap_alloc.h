#ifndef CELESTEOS_HEAP_ALLOC_H
#define CELESTEOS_HEAP_ALLOC_H

#include <stdbool.h>
#include <stddef.h>

#include "vmm.h"

bool initialize_heap(page_table_t* kernel_pml4);
void* kmalloc(size_t size);
void kfree(void* ptr);

#endif
