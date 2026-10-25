#include <limine.h>

#include <kernel/fb.h>
#include <kernel/gdt.h>
#include <kernel/idt.h>
#include <kernel/pmap.h>
#include <kernel/pmm.h>
#include <klog.h>

static void halt_forever(void) {
    for (;;) {
        __asm__ volatile("hlt");
    }
}

/* Limine requests */
__attribute__((used, section(".limine_requests"))) static volatile uint64_t limine_base_revision[] =
    LIMINE_BASE_REVISION(2);

// Limine Requests Start Marker
__attribute__((
    used, section(".limine_requests_start"))) static volatile uint64_t limine_requests_start[] =
    LIMINE_REQUESTS_START_MARKER;

__attribute__((
    used, section(".limine_requests"))) static volatile struct limine_hhdm_request hhdm_request = {
    .id = LIMINE_HHDM_REQUEST_ID,
    .revision = 0,
    .response = NULL,
};

__attribute__((
    used,
    section(".limine_requests"))) static volatile struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST_ID,
    .revision = 0,
    .response = NULL,
};

__attribute__((used, section(".limine_requests"))) static volatile struct limine_framebuffer_request
    framebuffer_request = {
        .id = LIMINE_FRAMEBUFFER_REQUEST_ID,
        .revision = 0,
        .response = NULL,
    };

// Limine Requests End Marker
__attribute__((used,
               section(".limine_requests_end"))) static volatile uint64_t limine_requests_end[] =
    LIMINE_REQUESTS_END_MARKER;

void kmain(void) {
    if (LIMINE_BASE_REVISION_SUPPORTED(limine_base_revision) == false) {
        klog(KLOG_ERROR, "Limine base revision not supported!\n");
        halt_forever();
    }
    uint64_t hhdm_base = hhdm_request.response->offset;
    struct limine_memmap_response *memmap_response = memmap_request.response;

    struct limine_framebuffer *fb = framebuffer_request.response->framebuffers[0];
    if (fb->address == NULL || fb->bpp != 32 || fb->width < 8 || fb->height < 16) {
        klog(KLOG_ERROR, "kmain: Unsupported framebuffer format or dimensions!\n");
        halt_forever();
    }

    /* Initialize kernel mode framebuffer */
    fb_init(&global_framebuffer, fb);
    klog(KLOG_INFO, "kmain: Framebuffer initialized successfully.\n");

    if (hhdm_request.response == NULL || memmap_request.response == NULL) {
        klog(KLOG_ERROR, "kmain: Limine HHDM/memory mapping not available!\n");
        halt_forever();
    }

    /* Minimal boot sequence */
    gdt_init();
    idt_init();
    isr_install();

    pmm_init(memmap_response, hhdm_base);
    klog(KLOG_INFO, "kmain: PMM initialized successfully.\n");

    if (pmap_init(hhdm_base) != 0) {
        klog(KLOG_ERROR, "kmain: Failed to initialize page map!\n");
        halt_forever();
    }
    klog(KLOG_INFO, "kmain: Page map initialized successfully.\n");

    /* TODO: Usermode jump */

    halt_forever();
}