#include "task.h"

#include <libkernel.h>
#include <string.h>

#include "memory/pmm.h"

tss_t kernel_tss __attribute__((aligned(16)));

static uint8_t kstack[16384] __attribute__((aligned(16)));
static uint8_t df_stack[16384] __attribute__((aligned(16)));
static uint8_t pf_stack[16384] __attribute__((aligned(16)));

/* Base address for the user stack, place it at 0x0000700000000000 */
#define USER_STACK_BASE 0x0000700000000000

/* Number of pages to allocate for the user stack */
#define USER_STACK_PAGES 4

void tss_initialize(void) {
	memset(&kernel_tss, 0, sizeof(kernel_tss));
	kernel_tss.io_map_base = sizeof(tss_t);
	kernel_tss.rsp0 = (uintptr_t)kstack + sizeof(kstack);

	// IST1 for double fault (#DF)
	kernel_tss.ist[0] = (uintptr_t)df_stack + sizeof(df_stack);

	// IST2 for page fault (#PF)
	kernel_tss.ist[1] = (uintptr_t)pf_stack + sizeof(pf_stack);
}

void tss_prepare_usermode(uintptr_t kernel_stack) { kernel_tss.rsp0 = kernel_stack; }

bool tss_jump_to_usermode(uintptr_t entry_point, page_table_t *root, uint64_t hhdm_base) {
	void *stack_frames;

	if (pmm_alloc_pages(USER_STACK_PAGES, &stack_frames) != 0) {
		klog(KLOG_ERROR, "task: Unable to allocate user stack.\n");
		return false;
	}

	memset((void *)((uintptr_t)stack_frames + hhdm_base), 0, USER_STACK_PAGES * PMAP_PAGE_SIZE);
	for (size_t page = 0; page < USER_STACK_PAGES; ++page) {
		if (pmap_map(root, USER_STACK_BASE + page * PMAP_PAGE_SIZE,
		             (uintptr_t)stack_frames + page * PMAP_PAGE_SIZE,
		             PAGE_USER | PAGE_READ_WRITE) != 0) {
			for (size_t mapped = 0; mapped < page; ++mapped) {
				pmap_unmap(root, USER_STACK_BASE + mapped * PMAP_PAGE_SIZE);
			}
			pmm_free_pages(stack_frames, USER_STACK_PAGES);
			klog(KLOG_ERROR, "task: Unable to map user stack.\n");
			return false;
		}
	}
	uintptr_t stack_top = USER_STACK_BASE + USER_STACK_PAGES * PMAP_PAGE_SIZE;
	uintptr_t kstack_top = (uintptr_t)kstack + sizeof(kstack);

	// Set kernel stack pointer top in the TSS, so Ring 3 can switch to the correct stack on
	// interrupts.
	tss_prepare_usermode(kstack_top);

	if (pmap_switch_pml4(root) != 0) {
		klog(KLOG_ERROR, "task: Unable to switch to the user page table.\n");
		return false;
	}

	// Switch to user mode!
	tss_switch_ring3(entry_point, stack_top);
	__builtin_unreachable();
}
