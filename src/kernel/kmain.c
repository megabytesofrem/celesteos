#include <kernel/drivers/framebuffer.h>
#include <kernel/gdt.h>
#include <kernel/idt.h>
#include <kernel/isr.h>
#include <kernel/mach/loader.h>
#include <kernel/memory/kmalloc.h>
#include <kernel/memory/pmm.h>
#include <kernel/memory/vm_map.h>
#include <kernel/memory/vmm.h>
#include <kernel/pic.h>
#include <kernel/ports.h>
#include <kernel/task.h>

#include <libkernel.h>
#include <limine.h>
#include <string.h>

#include <stdint.h>

// Boot logo
#include <kernel/drivers/boot_logo.h>

// Mach-O test payload
#include <kernel/mach/macho_test.h>

/* Macro so I can avoid typing this over and over again */
#define HCF \
    while (true) { \
        __asm__ volatile("hlt"); \
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
                    .response = NULL};

__attribute__((
    used,
    section(".limine_requests"))) static volatile struct limine_memmap_request
    memmap_request = {.id = LIMINE_MEMMAP_REQUEST_ID,
                      .revision = 0,
                      .response = NULL};

__attribute__((
    used,
    section(
        ".limine_requests"))) static volatile struct limine_framebuffer_request
    framebuffer_request = {.id = LIMINE_FRAMEBUFFER_REQUEST_ID,
                           .revision = 0,
                           .response = NULL};
// Limine Requests End Marker
__attribute__((used, section(".limine_requests_end"))) static volatile uint64_t
    limine_requests_end[] = LIMINE_REQUESTS_END_MARKER;

/**
 * Kernel main entry point.
 */
void kmain(void) {
    // Limine boilerplate
    if (LIMINE_BASE_REVISION_SUPPORTED(limine_base_revision) == false) {
        klog(KLOG_ERROR, "Limine base revision not supported!\n");
        HCF
    }

    // Check if the Higher Half Direct Mapping (HHDM) is available
    if (hhdm_request.response == NULL ||
        memmap_request.response == NULL) {
        klog(KLOG_ERROR, "Limine HHDM/memory mapping not available!\n");
        HCF
    }

    // Store the HHDM base address for later use
    uint64_t hhdm_base = hhdm_request.response->offset;
    struct limine_memmap_response* memmap_response = memmap_request.response;

    /* CPU Initialization */

    // Initialize the Global Descriptor Table (GDT)
    gdt_initialize();

    // Initialize the Interrupt Descriptor Table (IDT)
    idt_initialize();

    // Install ISRs
    isr_install();

    /* Drivers Initialization */

    // Initialize the Framebuffer driver
    if (framebuffer_request.response == NULL ||
        framebuffer_request.response->framebuffer_count == 0 ||
        framebuffer_request.response->framebuffers == NULL ||
        framebuffer_request.response->framebuffers[0] == NULL) {
        klog(KLOG_ERROR, "kmain: Limine framebuffer not available!\n");
        HCF
    }

    struct limine_framebuffer* fb = framebuffer_request.response->framebuffers[0];
    if (fb->address == NULL || fb->bpp != 32 || fb->width < 8 ||
        fb->height < 16) {
        klog(KLOG_ERROR, "kmain: Unsupported framebuffer format or dimensions!\n");
        HCF
    }

    framebuffer_init(&global_framebuffer, fb);

    // Display the boot logo on the framebuffer
    framebuffer_draw_raw_image(&global_framebuffer, 0, 0, logo_header_data, logo_width, logo_height);

    /* Memory Initialization */

    if (pmm_initialize(memmap_response, hhdm_base) != KERN_SUCCESS) {
        klog(KLOG_ERROR, "kmain: Failed to initialize the PMM!\n");
        HCF
    }
    klog(KLOG_INFO, "kmain: PMM initialized.\n");

    if (vmm_initialize(hhdm_base) != KERN_SUCCESS) {
        klog(KLOG_ERROR, "kmain: Failed to initialize the VMM!\n");
        HCF
    }
    klog(KLOG_INFO, "kmain: VMM initialized.\n");

    // Initialize the kernel virtual memory map
    if (vm_map_initialize_kernel(vmm_get_pml4()) != KERN_SUCCESS) {
        klog(KLOG_ERROR, "kmain: Failed to initialize kernel VM map!\n");
        HCF
    }
    klog(KLOG_INFO, "kmain: Kernel VM map initialized.\n");

    if (heap_initialize(kernel_vm_map) != KERN_SUCCESS) {
        klog(KLOG_ERROR, "kmain: Failed to initialize the heap!\n");
        HCF
    }
    volatile uint64_t* heap_test =
        (volatile uint64_t*)kmalloc(sizeof(uint64_t));
    if (heap_test == NULL) {
        klog(KLOG_ERROR, "kmain: Failed to allocate from the heap!\n");
        HCF
    }
    *heap_test = 0xCE1E57EULL;
    if (*heap_test != 0xCE1E57EULL) {
        klog(KLOG_ERROR, "kmain: Heap read/write test failed!\n");
        HCF
    }

    kfree((void*)heap_test);
    klog(KLOG_INFO, "kmain: Heap initialized and allocation verified.\n");

    /* Mach-O test parsing */
    macho_info macho_info;

    klog(KLOG_INFO, "kmain: Try to parse Mach-O test payload...\n");
    if (!macho_load(test_macho, vmm_get_pml4(), hhdm_base, &macho_info)) {
        klog(KLOG_ERROR, "kmain: Failed to load Mach-O test payload!\n");
        HCF
    }

    // Map 0x1000 to a newly allocated physical frame
    void* phys_frame;
    if (pmm_alloc_page(&phys_frame) != KERN_SUCCESS) {
        klog(KLOG_ERROR, "kmain: Failed to allocate a physical frame!\n");
        HCF
    }
    vmm_map(vmm_get_pml4(), 0x1000, (uintptr_t)phys_frame, PAGE_PRESENT | PAGE_USER | PAGE_READ_WRITE);

    /* User mode transition test */
    if (!tss_jump_to_usermode(macho_info.entrypoint, vmm_get_pml4(), hhdm_base)) {
        HCF
    }

    // Halt the CPU in an infinite loop
    HCF
}