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
__attribute__((used, section(".limine_requests"))) static volatile uint64_t limineBaseRevision[] =
    LIMINE_BASE_REVISION(2);

// Limine Requests Start Marker
__attribute__((
    used, section(".limine_requests_start"))) static volatile uint64_t limineRequestsStart[] =
    LIMINE_REQUESTS_START_MARKER;

__attribute__((
    used, section(".limine_requests"))) static volatile struct limine_hhdm_request hhdmRequest = {
    .id = LIMINE_HHDM_REQUEST_ID,
    .revision = 0,
    .response = NULL,
};

__attribute__((used,
    section(".limine_requests"))) static volatile struct limine_memmap_request memmapRequest = {
    .id = LIMINE_MEMMAP_REQUEST_ID,
    .revision = 0,
    .response = NULL,
};

__attribute__((used, section(".limine_requests"))) static volatile struct limine_framebuffer_request
    framebufferRequest = {
        .id = LIMINE_FRAMEBUFFER_REQUEST_ID,
        .revision = 0,
        .response = NULL,
};

// Limine Requests End Marker
__attribute__((
    used, section(".limine_requests_end"))) static volatile uint64_t limineRequestsEnd[] =
    LIMINE_REQUESTS_END_MARKER;

void kmain(void) {
    if (LIMINE_BASE_REVISION_SUPPORTED(limineBaseRevision) == false) {
        KeLog(KE_LOG_ERROR, "Limine base revision not supported!\n");
        halt_forever();
    }
    uint64_t hhdmBase = hhdmRequest.response->offset;
    struct limine_memmap_response *memmapResponse = memmapRequest.response;

    struct limine_framebuffer *fb = framebufferRequest.response->framebuffers[0];
    if (fb->address == NULL || fb->bpp != 32 || fb->width < 8 || fb->height < 16) {
        KeLog(KE_LOG_ERROR, "kmain: Unsupported framebuffer format or dimensions!\n");
        halt_forever();
    }

    /* Initialize kernel mode framebuffer */
    KeFbInitialize(&gKeFramebuffer, fb);

    if (hhdmRequest.response == NULL || memmapRequest.response == NULL) {
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

    KeInitPhysMemoryManager(memmapResponse, hhdmBase);
    KeLog(KE_LOG_INFO, "kmain: KeInitPhysMemoryManager successful.\n");

    if (KePageMapInit(hhdmBase) != 0) {
        KeLog(KE_LOG_ERROR, "kmain: Failed to initialize page map!\n");
        halt_forever();
    }
    KeLog(KE_LOG_INFO, "kmain: KePageMapInit successful.\n");

    PsTaskSubsystemInit(hhdmBase);
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