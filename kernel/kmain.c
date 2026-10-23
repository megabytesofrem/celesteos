#include <kernel/cpuid.h>
#include <kernel/ext2.h>
#include <kernel/gdt.h>
#include <kernel/idt.h>
#include <kernel/isr.h>

#include <kernel/fb.h>
#include <kernel/ipc/task.h>
#include <kernel/kmalloc.h>
#include <kernel/mach/loader.h>
#include <kernel/memory/pmap.h>
#include <kernel/memory/pmm.h>
#include <kernel/memory/vm_map.h>
#include <kernel/pic.h>
#include <kernel/ports.h>
#include <kernel/syscall.h>
#include <kernel/task.h>

#include <klog.h>
#include <limine.h>

#include <stdint.h>
#include <string.h>

// Boot logo
#include <boot_logo.h>

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

static ext2_fs_t initrd_fs;

/**
 * Load the initial ext2 ramdisk (initrd) early in the boot process to
 * bootstrap userspace `launchd` (PID 1).
 */
static void load_initrd(uint64_t hhdm_base) {
    klog(KLOG_INFO, "kmain: Initializing initrd...\n");
    if (module_request.response == NULL || module_request.response->modules == NULL) {
        klog(KLOG_ERROR, "kmain: No initrd modules found!\n");
        halt_forever();
    }

    struct limine_file *initrd_module = NULL;
    for (uint64_t i = 0; i < module_request.response->module_count; i++) {
        struct limine_file *module = module_request.response->modules[i];
        if (module != NULL && module->string != NULL &&
            strcmp(module->string, "ext2-initrd") == 0) {
            if (initrd_module != NULL) {
                klog(KLOG_ERROR, "kmain: Duplicate ext2-initrd modules\n");
                halt_forever();
            }
            initrd_module = module;
        }
    }
    if (initrd_module == NULL || initrd_module->address == NULL || initrd_module->size > SIZE_MAX) {
        klog(KLOG_ERROR, "kmain: Missing or invalid ext2-initrd module\n");
        halt_forever();
    }

    if (ext2_mount_image(&initrd_fs, initrd_module->address, (size_t)initrd_module->size) != 0) {
        klog(KLOG_ERROR, "kmain: ext2 initrd mount failed!\n");
        halt_forever();
    }
    klog(KLOG_INFO, "kmain: ext2 initrd ready (block size %u, groups %u)\n", initrd_fs.block_size,
         initrd_fs.group_count);

    // Locate /usr/libexec/launchd
    uint32_t launchd_inode = ext2_lookup_path(&initrd_fs, "/usr/libexec/launchd");
    if (launchd_inode == 0) {
        klog(KLOG_ERROR, "kmain: Failed to locate /usr/libexec/launchd in initrd!\n");
        halt_forever();
    }

    // Load the launchd inode
    ext2_inode_t launchd;
    if (ext2_read_inode(&initrd_fs, launchd_inode, &launchd) != 0) {
        klog(KLOG_ERROR, "kmain: Failed to read /usr/libexec/launchd inode!\n");
        halt_forever();
    }

    if ((launchd.mode & EXT2_INODE_TYPE_MASK) != EXT2_INODE_REGULAR ||
        launchd.size < sizeof(struct mach_header_64)) {
        klog(KLOG_ERROR, "kmain: launchd is not a valid regular executable file\n");
        halt_forever();
    }

    // Allocate a buffer to hold the launchd file contents
    uint8_t *launchd_data = kmalloc(launchd.size);
    if (launchd_data == NULL) {
        halt_forever();
    }

    // Read the launchd file contents into memory
    if (ext2_read_file(&initrd_fs, &launchd, launchd_data, launchd.size) != 0) {
        kfree(launchd_data);
        klog(KLOG_ERROR, "kmain: Failed to read launchd contents\n");
        halt_forever();
    }
    page_table_t *user_pml4 = pmap_create_user_pml4(hhdm_base);
    if (user_pml4 == NULL) {
        kfree(launchd_data);
        klog(KLOG_ERROR, "kmain: Failed to create launchd address space\n");
        halt_forever();
    }

    // Load the Mach-O image into the user address space
    struct macho_info launchd_info;
    kern_return_t result = macho_load(launchd_data, user_pml4, hhdm_base, &launchd_info);
    kfree(launchd_data);
    if (result != KERN_SUCCESS) {
        klog(KLOG_ERROR, "kmain: Failed to load launchd Mach-O image (%d)\n", result);
        halt_forever();
    }
    klog(KLOG_INFO, "kmain: Launching launchd at 0x%p\n", (void *)launchd_info.entrypoint);

    // Ring 3 transition: jump to user mode with the loaded launchd image
    if (tss_jump_to_usermode(launchd_info.entrypoint, launchd_info.stack_pointer, user_pml4,
                             hhdm_base) != 0) {
        klog(KLOG_ERROR, "kmain: Failed to enter launchd user mode\n");
        halt_forever();
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

    if (syscall_init(kernel_tss.rsp0) != 0) {
        klog(KLOG_ERROR, "kmain: Failed to initialize system calls!\n");
        halt_forever();
    }

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

    if (task_initialize_kernel() != KERN_SUCCESS) {
        klog(KLOG_ERROR, "kmain: Failed to initialize the bootstrap IPC task!\n");
        halt_forever();
    }

    /* INITRD */
    load_initrd(hhdm_base);

    halt_forever();
}