#include <cstdint>

#include "../limine.h"
#include "cpu/gdt.h"
#include "cpu/idt.h"
#include "cpu/ports.h"
#include "memory/pmm.h"

/* Macro so I can avoid typing this over and over again */
#define FOREVER              \
    while (true) {           \
        asm volatile("hlt"); \
    }

/* Limine requests */
__attribute__((used, section(".limine_requests"))) static volatile uint64_t limine_base_revision[] = LIMINE_BASE_REVISION(2);

// Limine Requests Start Marker
__attribute__((used, section(".limine_requests_start"))) static volatile uint64_t limine_requests_start[] = LIMINE_REQUESTS_START_MARKER;

// Limine Requests
__attribute__((used, section(".limine_requests"))) static volatile struct limine_hhdm_request hhdm_request = {
    .id = LIMINE_HHDM_REQUEST_ID,
    .revision = 0,
    .response = nullptr};

__attribute__((used, section(".limine_requests"))) static volatile struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST_ID,
    .revision = 0,
    .response = nullptr};

// Limine Requests End Marker
__attribute__((used, section(".limine_requests_end"))) static volatile uint64_t limine_requests_end[] = LIMINE_REQUESTS_END_MARKER;

/**
 * Kernel main entry point.
 */
extern "C" void kmain(void) {
    // Limine boilerplate
    if (LIMINE_BASE_REVISION_SUPPORTED(limine_base_revision) == false) {
        e9_write("[ERROR] Limine base revision not supported!\n");
        FOREVER
    }

    // Check if the Higher Half Direct Mapping (HHDM) is available
    if (hhdm_request.response == nullptr || memmap_request.response == nullptr) {
        e9_write("[ERROR] Limine HHDM/memory mapping not available!\n");
        FOREVER
    }

    // Store the HHDM base address for later use
    uint64_t hhdm_base = hhdm_request.response->offset;
    limine_memmap_response* memmap_response = memmap_request.response;

    // Initialize the Global Descriptor Table (GDT)
    cpu::initialize_gdt();

    // Initialize the Interrupt Descriptor Table (IDT)
    cpu::initialize_idt();

    // Write something to the QEMU debug port (0xE9)
    e9_write("[INFO] Debug port initialized.\n");

    // Initialize the PMM
    memory::PhysMemoryManager::instance().initialize(memmap_response, hhdm_base);

    // Test the PMM by allocating a page
    void* test_page = memory::PhysMemoryManager::instance().alloc_page();
    if (test_page == nullptr) {
        e9_write("[ERROR] Failed to allocate a test page!\n");
        FOREVER
    }

    e9_write("[INFO] Test page allocated successfully.\n");
    e9_write("[INFO] Test page address: 0x");
    // Print the test page address in hexadecimal
    for (int i = 60; i >= 0; i -= 4) {
        uint8_t nibble = (reinterpret_cast<uint64_t>(test_page) >> i) & 0xF;
        char c = (nibble < 10) ? ('0' + nibble) : ('A' + nibble - 10);
        e9_write(&c);
    }
    e9_write("\n");

    e9_write("[INFO] Entering main kernel loop.\n");

    // Welcome to Ring 0 C++!
    FOREVER
}