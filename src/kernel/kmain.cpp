#include <cpu/gdt.h>
#include <cpu/idt.h>
#include <cpu/ports.h>
#include <drivers/framebuffer.h>
#include <macho.h>
#include <memory/pmm.h>
#include <memory/vmm.h>
#include <memory/heap_alloc.h>

#include <stdlib/string.h>
#include <libkernel.h>
#include <limine.h>

#include <cstdint>

// Mach-O test payload
#include "macho_test.h"

/* Macro so I can avoid typing this over and over again */
#define HCF \
    while (true) { \
        asm volatile("hlt"); \
    }

/* Limine requests */
__attribute__((used, section(".limine_requests"))) static volatile uint64_t
    limine_base_revision[] = LIMINE_BASE_REVISION(2);

// Limine Requests Start Marker
__attribute__((used,
               section(".limine_requests_start"))) static volatile uint64_t
    limine_requests_start[] = LIMINE_REQUESTS_START_MARKER;

// Limine Requests
__attribute__((
    used,
    section(".limine_requests"))) static volatile struct limine_hhdm_request
    hhdm_request = {.id = LIMINE_HHDM_REQUEST_ID,
                    .revision = 0,
                    .response = nullptr};

__attribute__((
    used,
    section(".limine_requests"))) static volatile struct limine_memmap_request
    memmap_request = {.id = LIMINE_MEMMAP_REQUEST_ID,
                      .revision = 0,
                      .response = nullptr};

__attribute__((
    used,
    section(
        ".limine_requests"))) static volatile struct limine_framebuffer_request
    framebuffer_request = {.id = LIMINE_FRAMEBUFFER_REQUEST_ID,
                           .revision = 0,
                           .response = nullptr};

// Limine Requests End Marker
__attribute__((used, section(".limine_requests_end"))) static volatile uint64_t
    limine_requests_end[] = LIMINE_REQUESTS_END_MARKER;

/**
 * Kernel main entry point.
 */
extern "C" void kmain(void) {
    // Limine boilerplate
    if (LIMINE_BASE_REVISION_SUPPORTED(limine_base_revision) == false) {
        klog(LOG_ERROR, "Limine base revision not supported!\n");
        HCF
    }

    // Check if the Higher Half Direct Mapping (HHDM) is available
    if (hhdm_request.response == nullptr ||
        memmap_request.response == nullptr) {
        klog(LOG_ERROR, "Limine HHDM/memory mapping not available!\n");
        HCF
    }

    // Store the HHDM base address for later use
    uint64_t hhdm_base = hhdm_request.response->offset;
    limine_memmap_response* memmap_response = memmap_request.response;

    /* CPU Initialization */

    // Initialize the Global Descriptor Table (GDT)
    cpu::initialize_gdt();

    // Initialize the Interrupt Descriptor Table (IDT)
    cpu::initialize_idt();

    /* Drivers Initialization */

    // Initialize the Framebuffer driver
    auto& framebuffer = drivers::Framebuffer::instance();
    if (framebuffer_request.response == nullptr ||
        framebuffer_request.response->framebuffer_count == 0 ||
        framebuffer_request.response->framebuffers == nullptr ||
        framebuffer_request.response->framebuffers[0] == nullptr) {
        klog(LOG_ERROR, "kmain: Limine framebuffer not available!\n");
        HCF
    }
    limine_framebuffer* fb = framebuffer_request.response->framebuffers[0];
    if (fb->address == nullptr || fb->bpp != 32 || fb->width < 8 ||
        fb->height < 16) {
        klog(LOG_ERROR, "kmain: Unsupported framebuffer format or dimensions!\n");
        HCF
    }
    framebuffer.initialize(fb);

    /* Memory Initialization */

    auto& pmm = memory::PhysMemoryManager::instance();
    pmm.initialize(memmap_response, hhdm_base);
    klog(LOG_INFO, "kmain: PMM initialized.\n");

    auto& vmm = memory::VirtualMemoryManager::instance();
    vmm.initialize(hhdm_base);
    klog(LOG_INFO, "kmain: VMM initialized.\n");

    // Keep Limine's existing kernel, stack, and HHDM mappings active.
    uintptr_t active_cr3;
    asm volatile("mov %%cr3, %0" : "=r"(active_cr3));
    auto* active_pml4 = reinterpret_cast<memory::PageTable*>(
        (active_cr3 & 0x000FFFFFFFFFF000ULL) + hhdm_base);
    if (!memory::initialize_heap(active_pml4)) {
        klog(LOG_ERROR, "kmain: Failed to initialize the heap!\n");
        HCF
    }
    volatile uint64_t* heap_test = reinterpret_cast<volatile uint64_t*>(memory::kmalloc(sizeof(uint64_t)));
    if (heap_test == nullptr) {
        klog(LOG_ERROR, "kmain: Failed to allocate from the heap!\n");
        HCF
    }
    *heap_test = 0xCE1E57EULL;
    if (*heap_test != 0xCE1E57EULL) {
        klog(LOG_ERROR, "kmain: Heap read/write test failed!\n");
        HCF
    }

    memory::kfree(const_cast<uint64_t*>(heap_test));
    klog(LOG_INFO, "kmain: Heap initialized and allocation verified.\n");

    // Allocate a test page from the PMM to verify initialization
    void* test_page = pmm.alloc_page();
    if (test_page == nullptr) {
        klog(LOG_ERROR, "kmain: Failed to allocate the test page!\n");
        HCF
    }

    klog(LOG_INFO, "kmain: Test page allocated successfully.\n");
    klog(LOG_INFO, "       Test page address: 0x%p\n", test_page);

    /* Mach-O test parsing */
    macho::MachOInfo macho_info;

    klog(LOG_INFO, "kmain: Try to parse Mach-O test payload...\n");
    macho::parse_macho(test_macho, macho_info);

    // Halt the CPU in an infinite loop
    HCF
}