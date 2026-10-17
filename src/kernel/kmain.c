#include <kernel/drivers/framebuffer.h>
#include <kernel/gdt.h>
#include <kernel/idt.h>
#include <kernel/isr.h>
#include <kernel/mach/loader.h>
#include <kernel/memory/kmalloc.h>
#include <kernel/memory/pmap.h>
#include <kernel/memory/pmm.h>
#include <kernel/memory/vm_map.h>
#include <kernel/pic.h>
#include <kernel/ports.h>
#include <kernel/tar.h>
#include <kernel/task.h>

#include <libkernel.h>
#include <limine.h>

#include <stdint.h>

// Boot logo
#include <kernel/drivers/boot_logo.h>

static void halt_forever(void) {
    for (;;) {
        __asm__ volatile("hlt");
    }
}

/* Limine requests */
__attribute__((used, section(".limine_requests"))) static volatile uint64_t limine_base_revision[] =
    LIMINE_BASE_REVISION(2);

// Limine Requests Start Marker
__attribute__((used, section(".limine_requests_start"))) static volatile uint64_t limine_requests_start[] =
    LIMINE_REQUESTS_START_MARKER;

// Limine Requests
__attribute__((used, section(".limine_requests"))) static volatile struct limine_hhdm_request hhdm_request = {
    .id = LIMINE_HHDM_REQUEST_ID,
    .revision = 0,
    .response = NULL,
};

__attribute__((used,
               section(".limine_requests"))) static volatile struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST_ID,
    .revision = 0,
    .response = NULL,
};

__attribute__((used,
               section(".limine_requests"))) static volatile struct limine_module_request module_request = {
    .id = LIMINE_MODULE_REQUEST_ID,
    .revision = 0,
    .response = NULL,
};

__attribute__((
    used,
    section(".limine_requests"))) static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST_ID,
    .revision = 0,
    .response = NULL,
};

// Limine Requests End Marker
__attribute__((used, section(".limine_requests_end"))) static volatile uint64_t limine_requests_end[] =
    LIMINE_REQUESTS_END_MARKER;

/**
 * Kernel main entry point.
 */
void kmain(void) {
    // Limine boilerplate
    if (LIMINE_BASE_REVISION_SUPPORTED(limine_base_revision) == false) {
        klog(KLOG_ERROR, "Limine base revision not supported!\n");
        halt_forever();
    }

    if (hhdm_request.response == NULL || memmap_request.response == NULL) {
        klog(KLOG_ERROR, "Limine HHDM/memory mapping not available!\n");
        halt_forever();
    }

    uint64_t hhdm_base = hhdm_request.response->offset;
    struct limine_memmap_response *memmap_response = memmap_request.response;

    /* CPU Initialization */
    // Initialize the Global Descriptor Table (GDT)
    gdt_initialize();

    // Initialize the Interrupt Descriptor Table (IDT)
    idt_initialize();

    // Install ISRs
    isr_install();

    /* Framebuffer Initialization */
    if (framebuffer_request.response == NULL || framebuffer_request.response->framebuffer_count == 0 ||
        framebuffer_request.response->framebuffers == NULL ||
        framebuffer_request.response->framebuffers[0] == NULL) {
        klog(KLOG_ERROR, "kmain: Limine framebuffer not available!\n");
        halt_forever();
    }

    struct limine_framebuffer *fb = framebuffer_request.response->framebuffers[0];
    if (fb->address == NULL || fb->bpp != 32 || fb->width < 8 || fb->height < 16) {
        klog(KLOG_ERROR, "kmain: Unsupported framebuffer format or dimensions!\n");
        halt_forever();
    }

    framebuffer_init(&global_framebuffer, fb);

    framebuffer_draw_raw_image(&global_framebuffer, 0, 0, logo_header_data, logo_width, logo_height);

    /* Memory Initialization */

    pmm_initialize(memmap_response, hhdm_base);
    klog(KLOG_INFO, "kmain: PMM initialized.\n");

    if (pmap_initialize(hhdm_base) != 0) {
        klog(KLOG_ERROR, "kmain: Failed to initialize the pmap!\n");
        halt_forever();
    }
    klog(KLOG_INFO, "kmain: Pmap initialized.\n");

    if (vm_map_initialize_kernel(pmap_get_pml4()) != 0) {
        klog(KLOG_ERROR, "kmain: Failed to initialize kernel VM map!\n");
        halt_forever();
    }
    klog(KLOG_INFO, "kmain: Kernel VM map initialized.\n");

    if (heap_initialize(kernel_vm_map) != 0) {
        klog(KLOG_ERROR, "kmain: Failed to initialize the heap!\n");
        halt_forever();
    }
    volatile uint64_t *heap_test = (volatile uint64_t *)kmalloc(sizeof(uint64_t));
    if (heap_test == NULL) {
        klog(KLOG_ERROR, "kmain: Failed to allocate from the heap!\n");
        halt_forever();
    }

    /* INITRD */
    klog(KLOG_INFO, "kmain: Initializing INITRD...\n");
    if (module_request.response == NULL || module_request.response->module_count < 1) {
        klog(KLOG_ERROR, "kmain: No INITRD modules found!\n");
        halt_forever();
    }

    struct limine_file *initrd_module = module_request.response->modules[0];
    klog(KLOG_INFO,
         "kmain: INITRD module found at %p (size: %u bytes)\n",
         initrd_module->address,
         initrd_module->size);

    tar_archive_t initrd = {.base = initrd_module->address, .size = initrd_module->size};

    /* Read our stub launchd binary from the INITRD */
    tar_file_t launchd_file;
    if (tar_lookup_file(&initrd, "/usr/libexec/launchd", &launchd_file) != 0) {
        klog(KLOG_ERROR, "kmain: Failed to find /usr/libexec/launchd in the INITRD!\n");
        halt_forever();
    }

    klog(KLOG_INFO,
         "kmain: launchd file found at %p (size: %u bytes)\n",
         launchd_file.base,
         launchd_file.size);

    /* Load the Mach-O binary for launchd from the INITRD */
    page_table_t *user_pml4 = pmap_create_user_pml4(hhdm_base);

    struct macho_info launchd_info;
    if (macho_load(launchd_file.base, user_pml4, hhdm_base, &launchd_info) != KERN_SUCCESS) {
        klog(KLOG_ERROR, "kmain: Failed to load launchd Mach-O binary!\n");
        halt_forever();
    }

    klog(KLOG_INFO,
         "kmain: /usr/libexec/launchd loaded successfully. Entry point at 0x%p\n",
         (void *)launchd_info.entrypoint);

    klog(KLOG_INFO,
         "kmain: Launching /usr/libexec/launchd at entry point 0x%p\n",
         (void *)launchd_info.entrypoint);

    // Jump to the entry point of launchd
    tss_jump_to_usermode(launchd_info.entrypoint, user_pml4, hhdm_base);

    halt_forever();
}