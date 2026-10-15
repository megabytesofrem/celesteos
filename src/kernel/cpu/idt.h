#ifndef CELESTEOS_IDT_H
#define CELESTEOS_IDT_H

#include <cstdint>

namespace cpu {

#pragma pack(push, 1)

/**
 * An entry in the Interrupt Descriptor Table (IDT).
 */
struct alignas(16) IDTEntry {
    uint16_t offset_low;    /* low 16 bits of the offset */
    uint16_t selector;      /* code segment selector */
    uint8_t ist;            /* bits 0..2 hold the Interrupt Stack Table index */
    uint8_t type_attr;      /* type and attributes of the interrupt gate */
    uint16_t offset_middle; /* middle 16 bits of the offset */
    uint32_t offset_high;   /* high 32 bits of the offset */
    uint32_t zero;          /* reserved, set to zero */
};

/**
 * Pointer to the Interrupt Descriptor Table (IDT)
 */
struct IDTPointer {
    uint16_t limit; /* size of the IDT in bytes - 1 */
    uint64_t base;  /* address of the first element in the IDT */
};

#pragma pack(pop)

void idt_set_entry(int entry, uint64_t handler, int type_attr);
void initialize_idt();

#endif  // CELESTEOS_IDT_H

}  // namespace cpu