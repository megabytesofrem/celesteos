#ifndef CELESTEOS_IDT_H
#define CELESTEOS_IDT_H

#include <stddef.h>
#include <stdint.h>

/**
 * An entry in the Interrupt Descriptor Table (IDT).
 */
typedef struct __attribute__((packed)) idt_entry_t {
	uint16_t offset_low;    /* low 16 bits of the offset */
	uint16_t selector;      /* code segment selector */
	uint8_t ist;            /* bits 0..2 hold the Interrupt Stack Table index */
	uint8_t type_attr;      /* type and attributes of the interrupt gate */
	uint16_t offset_middle; /* middle 16 bits of the offset */
	uint32_t offset_high;   /* high 32 bits of the offset */
	uint32_t zero;          /* reserved, set to zero */
} idt_entry_t;

/**
 * Packed 10-byte LIDT operand: 16-bit limit followed by a 64-bit base.
 */
typedef struct __attribute__((packed)) idt_pointer_t {
	uint16_t limit; /* size of the IDT in bytes - 1 */
	uint64_t base;  /* address of the first element in the IDT */
} idt_pointer_t;

_Static_assert(sizeof(idt_entry_t) == 16, "IDT entries must be 16 bytes");
_Static_assert(sizeof(idt_pointer_t) == 10, "IDTR operands must be 10 bytes");
_Static_assert(offsetof(idt_pointer_t, base) == 2, "IDTR base must follow the 16-bit limit");

typedef void (*idt_handler_t)(void);

/**
 * Initialize the Interrupt Descriptor Table (IDT).
 */
void idt_initialize(void);

/**
 * Set an entry in the IDT to point to a specific interrupt handler.
 */
void idt_set_entry(int entry, idt_handler_t handler, int type_attr);

/**
 * Set an entry in the IDT to point to a specific interrupt handler with a
 * given Interrupt Stack Table (IST) index.
 */
void idt_set_entry_ist(int entry, idt_handler_t handler, int type_attr, int ist_index);

void idt_load(void);

#endif // CELESTEOS_IDT_H
