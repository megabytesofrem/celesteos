#include <kernel/cpuid.h>
#include <kernel/gdt.h>
#include <kernel/idt.h>
#include <kernel/isr.h>

#include <kernel/drivers/framebuffer.h>
#include <kernel/initrd.h>
#include <kernel/mach/loader.h>
#include <kernel/memory/kmalloc.h>
#include <kernel/memory/pmap.h>
#include <kernel/memory/pmm.h>
#include <kernel/memory/vm_map.h>
#include <kernel/pic.h>
#include <kernel/ports.h>
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
__attribute__((
    used, section(".limine_requests_start"))) static volatile uint64_t limine_requests_start[] =
    LIMINE_REQUESTS_START_MARKER;

// Limine Requests
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

__attribute__((
    used,
    section(".limine_requests"))) static volatile struct limine_module_request module_request = {
    .id = LIMINE_MODULE_REQUEST_ID,
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

void detect_supported_cpu_features(void) {
    uint32_t eax, edx;
    cpuid(1, &eax, &edx);

    if ((edx & CPUID_FEAT_EDX_FPU)) {
        klog(KLOG_INFO, "detect_supported_cpu_features: FPU supported.\n");
    }
    if ((edx & CPUID_FEAT_EDX_PAE)) {
        klog(KLOG_INFO, "detect_supported_cpu_features: PAE supported.\n");
    }
    if ((edx & CPUID_FEAT_EDX_SSE)) {
        klog(KLOG_INFO, "detect_supported_cpu_features: SSE supported.\n");
    }
    if ((edx & CPUID_FEAT_EDX_SSE2)) {
        klog(KLOG_INFO, "detect_supported_cpu_features: SSE2 supported.\n");
    }

    uint32_t ecx;
    cpuid(1, &eax, &ecx);

    if ((ecx & CPUID_FEAT_ECX_SSE3)) {
        klog(KLOG_INFO, "detect_supported_cpu_features: SSE3 supported.\n");
    }
}

void draw_greeter(void) {
    if (framebuffer_request.response == NULL ||
        framebuffer_request.response->framebuffer_count == 0 ||
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
    framebuffer_draw_raw_image(&global_framebuffer, 0, 0, logo_header_data, logo_width,
                               logo_height);
}

/**
 * Load the initrd, in-memory file system from a tar archive.
 *
 * The initrd file system is built by scripts/build_initrd.sh.
 */
void load_initrd(uint64_t hhdm_base, page_table_t *user_pml4, struct macho_info *out_macho) {
    klog(KLOG_INFO, "kmain: Initializing initrd...\n");
    if (module_request.response == NULL || module_request.response->module_count < 1) {
        klog(KLOG_ERROR, "kmain: No initrd modules found!\n");
        halt_forever();
    }

    struct limine_file *initrd_module = module_request.response->modules[0];
    klog(KLOG_INFO, "kmain: initrd module found at %p (size: %u bytes)\n", initrd_module->address,
         initrd_module->size);

    initrd_tar_archive_t initrd_tar = { .base = initrd_module->address,
                                        .size = initrd_module->size };

    // Construct an initrd ram filesystem
    struct initrd_filesystem initrd_fs;
    if (initrd_init_fs(&initrd_fs, NULL, 0) != KERN_SUCCESS) {
        klog(KLOG_ERROR, "kmain: Failed to initialize initrd filesystem!\n");
        halt_forever();
    }

    if (initrd_load_tar(&initrd_fs, &initrd_tar) != KERN_SUCCESS) {
        klog(KLOG_ERROR, "kmain: Failed to load initrd tar archive!\n");
        halt_forever();
    }

    klog(KLOG_INFO, "kmain: initrd filesystem tree:\n");
    kprintf("\n");
    initrd_print_tree(initrd_fs.root, NULL);

    initrd_file_t launchd_file;
    if (initrd_read_file(&initrd_fs, "/usr/libexec/launchd", &launchd_file.data,
                         &launchd_file.size) != KERN_SUCCESS) {
        klog(KLOG_ERROR, "kmain: Failed to read /usr/libexec/launchd from initrd!\n");
        halt_forever();
    }

    klog(KLOG_INFO, "kmain: launchd file found at %p (size: %u bytes)\n", launchd_file.data,
         launchd_file.size);

    /* Load the Mach-O binary for launchd from initrd */
    struct macho_info launchd_info;
    if (macho_load(launchd_file.data, user_pml4, hhdm_base, &launchd_info) != KERN_SUCCESS) {
        klog(KLOG_ERROR, "kmain: Failed to load /usr/libexec/launchd Mach-O binary!\n");
        halt_forever();
    }

    klog(KLOG_INFO, "kmain: /usr/libexec/launchd loaded successfully. Entry point at 0x%p\n",
         (void *)launchd_info.entrypoint);

    klog(KLOG_INFO, "kmain: Launching /usr/libexec/launchd at entry point 0x%p\n",
         (void *)launchd_info.entrypoint);

    if (out_macho != NULL) {
        *out_macho = launchd_info;
    }
}

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
    draw_greeter();

    /* CPU feature detection */
    detect_supported_cpu_features();

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
    page_table_t *user_pml4 = pmap_create_user_pml4(hhdm_base);

    struct macho_info launchd_info;
    load_initrd(hhdm_base, user_pml4, &launchd_info);

    // Jump to the entry point of launchd
    tss_jump_to_usermode(launchd_info.entrypoint, user_pml4, hhdm_base);

    halt_forever();
}