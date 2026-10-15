#include <cpu/gdt.h>
#include <cpu/idt.h>
#include <cpu/ports.h>
#include <drivers/framebuffer.h>
#include <memory/pmm.h>
#include <memory/vmm.h>

#include <stdlib/string.h>
#include <libkernel.h>
#include <limine.h>

#include <cstdint>
#include "pony.h"

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

    /* Memory Initialization */

    // Initialize the PMM
    auto& pmm = memory::PhysMemoryManager::instance();
    pmm.initialize(memmap_response, hhdm_base);
    klog(LOG_INFO, "kmain: PMM initialized.\n");

    // Initialize the Virtual Memory Manager (VMM)
    auto& vmm = memory::VirtualMemoryManager::instance();
    vmm.initialize(hhdm_base);
    klog(LOG_INFO, "kmain: VMM initialized.\n");

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

    // Allocate a test page from the PMM to verify initialization
    void* test_page = pmm.alloc_page();
    if (test_page == nullptr) {
        klog(LOG_ERROR, "kmain: Failed to allocate the test page!\n");
        HCF
    }

    klog(LOG_INFO, "kmain: Test page allocated successfully.\n");
    klog(LOG_INFO, "       Test page address: 0x%p\n", test_page);

    // Framebuffer test: render a pony
    klog(LOG_INFO, "kmain: Here is a pony to celebrate!\n");
    drivers::Framebuffer::instance().draw_gimp_image(15, 100, header_data, image_width, image_height);

    klog(LOG_INFO, "kmain: Pony rendered successfully :)\n");
    // Halt the CPU in an infinite loop
    HCF
}