#include <limine.h>

#include <kernel/fb.h>
#include <kernel/gdt.h>
#include <kernel/idt.h>
#include <kernel/tss.h>
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

__attribute__((used, section(".limine_requests"))) static volatile struct limine_framebuffer_request
    framebuffer_request = {
        .id = LIMINE_FRAMEBUFFER_REQUEST_ID,
        .revision = 0,
        .response = NULL,
    };

// Limine Requests End Marker
__attribute__((
    used, section(".limine_requests_end"))) static volatile uint64_t limine_requests_end[] =
    LIMINE_REQUESTS_END_MARKER;

void kmain(void) {
    if (LIMINE_BASE_REVISION_SUPPORTED(limine_base_revision) == false) {
        klog(KLOG_ERROR, "Limine base revision not supported!\n");
        halt_forever();
    }

    /* Initialize kernel mode framebuffer */
    struct limine_framebuffer *fb = framebuffer_request.response->framebuffers[0];
    if (fb->address == NULL || fb->bpp != 32 || fb->width < 8 || fb->height < 16) {
        klog(KLOG_ERROR, "kmain: Unsupported framebuffer format or dimensions!\n");
        halt_forever();
    }

    fb_init(&global_framebuffer, fb);
    klog(KLOG_INFO, "Framebuffer initialized successfully.\n");

    klog(KLOG_INFO, "Initializing GDT, IDT...\n");
    gdt_init();
    idt_init();
    isr_install();
    klog(KLOG_INFO, "GDT and IDT initialized successfully.\n");

    halt_forever();
}