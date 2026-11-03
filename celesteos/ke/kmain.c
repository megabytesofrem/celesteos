#include <limine.h>

#include <ke/fb.h>
#include <ke/gdt.h>
#include <ke/idt.h>
#include <ke/log.h>
#include <ke/page.h>
#include <ke/pmm.h>
#include <ps/endpoint.h>
#include <ps/pstask.h>
#include <ps/psthread.h>
#include <ps/sched.h>

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

__attribute__((used,
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
__attribute__((
    used, section(".limine_requests_end"))) static volatile uint64_t limine_requests_end[] =
    LIMINE_REQUESTS_END_MARKER;

void kmain(void) {
    if (LIMINE_BASE_REVISION_SUPPORTED(limine_base_revision) == false) {
        KeLog(KE_LOG_ERROR, "Limine base revision not supported!\n");
        halt_forever();
    }
    uint64_t hhdm_base = hhdm_request.response->offset;
    struct limine_memmap_response *memmap_response = memmap_request.response;

    struct limine_framebuffer *fb = framebuffer_request.response->framebuffers[0];
    if (fb->address == NULL || fb->bpp != 32 || fb->width < 8 || fb->height < 16) {
        KeLog(KE_LOG_ERROR, "kmain: Unsupported framebuffer format or dimensions!\n");
        halt_forever();
    }

    /* Initialize kernel mode framebuffer */
    KeFbInitialize(&g_keFramebuffer, fb);

    if (hhdm_request.response == NULL || memmap_request.response == NULL) {
        KeLog(KE_LOG_ERROR, "kmain: Limine HHDM/memory mapping not available!\n");
        halt_forever();
    }

    /* Minimal boot sequence */

    // Print kernel banner
    KePrintString(" .---. .----..-.   .----. .----..---. .----..----.  .----. \n");
    KePrintString("/  ___}| {_  | |   | {_  { {__ {_   _}| {_ /  {}  \\{ {__  \n");
    KePrintString("\\     }| {__ | `--.| {__ .-._} } | |  | {__\\      /.-._} }\n");
    KePrintString(" `---' `----'`----'`----'`----'  `-'  `----'`----' `----' \n");

    KeInitGDT();
    KeInitIDT();
    KeInstallISRHandlers();

    KeInitPhysMemoryManager(memmap_response, hhdm_base);
    KeLog(KE_LOG_INFO, "kmain: KeInitPhysMemoryManager successful.\n");

    if (KePageMapInit(hhdm_base) != 0) {
        KeLog(KE_LOG_ERROR, "kmain: Failed to initialize page map!\n");
        halt_forever();
    }
    KeLog(KE_LOG_INFO, "kmain: KePageMapInit successful.\n");

    PsTaskSubsystemInit(hhdm_base);
    KeLog(KE_LOG_INFO, "kmain: PsTaskSubsystemInit successful.\n");

    PsThreadSubsystemInit();
    KeLog(KE_LOG_INFO, "kmain: PsThreadSubsystemInit successful.\n");

    PsSchedulerInit();
    KeLog(KE_LOG_INFO, "kmain: PsSchedulerInit successful.\n");
    PsEndpointSubsystemInit();
    KeLog(KE_LOG_INFO, "kmain: PsEndpointSubsystemInit successful.\n");

    /* TODO: Usermode jump */

    halt_forever();
}