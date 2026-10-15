#pragma once

#include <cstdint>
#include <cstddef>

#include "vmm.h"

namespace memory {

// The supplied PML4 must be active so the heap is accessible immediately.
[[nodiscard]] bool initialize_heap(PageTable* kernel_pml4);
void* kmalloc(size_t size);
void kfree(void* ptr);

}  // namespace memory

void* operator new(size_t size);
void* operator new[](size_t size);
void operator delete(void* ptr) noexcept;
void operator delete[](void* ptr) noexcept;
void operator delete(void* ptr, size_t size) noexcept;
