#ifndef CELESTEOS_KMALLOC_H
#define CELESTEOS_KMALLOC_H

#include <stdbool.h>
#include <stddef.h>

#include "vmm.h"

bool initialize_heap(page_table_t* kernel_pml4);
void* kmalloc(size_t size);
void kfree(void* ptr);

#endif  // CELESTEOS_KMALLOC_H
