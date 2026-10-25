#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>

#include <kernel/gdt.h>
#include <kernel/ipc.h>
#include <kernel/ipc/scheduler.h>
#include <kernel/ipc/task.h>
#include <kernel/tss.h>

static bool gdt_loaded;
static bool tss_loaded;

void load_gdt(const gdt_pointer_t *ptr, uint16_t cs, uint16_t ds) {
    assert(cs == GDT_KERNEL_CS);
    assert(ds == GDT_KERNEL_DS);
    assert(ptr->limit == sizeof(gdt_t) - 1);

    const gdt_t *gdt = (const gdt_t *)(uintptr_t)ptr->base;
    uint64_t low = gdt->tss_low;
    uint64_t high = gdt->tss_high;
    uintptr_t base = ((low >> 16) & 0xFFFFFF) | (((low >> 56) & 0xFF) << 24) |
                     ((high & UINT64_C(0xFFFFFFFF)) << 32);
    uint64_t limit = (low & 0xFFFF) | (((low >> 48) & 0xF) << 16);

    assert(base == (uintptr_t)tss_get());
    assert(limit == sizeof(tss_t) - 1);
    assert(((low >> 40) & 0xFF) == 0x89);
    assert(((low >> 52) & 0xF) == 0);
    assert((high >> 32) == 0);
    assert(tss_get()->rsp0 != 0);
    gdt_loaded = true;
}

void load_tss(uint16_t selector) {
    assert(gdt_loaded);
    assert(selector == GDT_TSS);
    tss_loaded = true;
}

int main(void) {
    gdt_init();
    assert(tss_loaded);
    const tss_t *tss = tss_get();
    uint64_t bootstrap_top = tss->rsp0;
    uint64_t double_fault_top = tss->ist[TSS_IST_DOUBLE_FAULT - 1];
    uint64_t page_fault_top = tss->ist[TSS_IST_PAGE_FAULT - 1];

    assert((bootstrap_top & 0xF) == 0);
    assert(double_fault_top != 0 && (double_fault_top & 0xF) == 0);
    assert(page_fault_top != 0 && (page_fault_top & 0xF) == 0);
    assert(bootstrap_top != double_fault_top && bootstrap_top != page_fault_top);
    assert(double_fault_top != page_fault_top);
    assert(tss->io_map_base == sizeof(tss_t));
    assert(tss->rsp1 == 0 && tss->rsp2 == 0);
    assert(tss->reserved0 == 0 && tss->reserved1 == 0);
    assert(tss->reserved2 == 0 && tss->reserved3 == 0);
    for (size_t i = 2; i < 7; ++i) {
        assert(tss->ist[i] == 0);
    }

    const uint64_t invalid[] = {
        0, 0x1000, UINT64_C(0x0000800000000000), UINT64_C(0xFFFF7FFFFFFFFFF0),
        UINT64_C(0xFFFF800000000001), UINT64_MAX,
    };
    for (size_t i = 0; i < sizeof(invalid) / sizeof(invalid[0]); ++i) {
        assert(tss_prepare_user(invalid[i]) == -EINVAL);
        assert(tss->rsp0 == bootstrap_top);
    }

    assert(tss_prepare_user(UINT64_C(0xFFFFFFFF80004000)) == 0);
    assert(tss->rsp0 == UINT64_C(0xFFFFFFFF80004000));
    assert(tss->ist[TSS_IST_DOUBLE_FAULT - 1] == double_fault_top);
    assert(tss->ist[TSS_IST_PAGE_FAULT - 1] == page_fault_top);
    assert(tss->io_map_base == sizeof(tss_t));
    return 0;
}
