#include <limine.h>

#include <kernel/gdt.h>
#include <kernel/tss.h>

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

// Limine Requests End Marker
__attribute__((used,
               section(".limine_requests_end"))) static volatile uint64_t limine_requests_end[] =
    LIMINE_REQUESTS_END_MARKER;

void kmain(void) {
    gdt_init();

    halt_forever();
}