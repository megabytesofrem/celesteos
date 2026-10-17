#ifndef CELESTEOS_ISR_H
#define CELESTEOS_ISR_H

#include <stddef.h>
#include <stdint.h>

/*
 * CPU register state saved during an interrupt.
 */
struct cpu_registers {
	uint64_t cr2;
	uint64_t r15;
	uint64_t r14;
	uint64_t r13;
	uint64_t r12;
	uint64_t r11;
	uint64_t r10;
	uint64_t r9;
	uint64_t r8;
	uint64_t rbp;
	uint64_t rdi;
	uint64_t rsi;
	uint64_t rdx;
	uint64_t rcx;
	uint64_t rbx;
	uint64_t rax;

	uint64_t int_no, err_code;
	uint64_t rip, cs, rflags, rsp, ss;
};

_Static_assert(offsetof(struct cpu_registers, int_no) == 128,
               "Interrupt number must follow CR2 and 15 saved registers");
_Static_assert(sizeof(struct cpu_registers) == 184,
               "Interrupt frame must match the assembly stack layout");

/**
 * Install the interrupt service routines (ISRs) for handling CPU exceptions
 * and hardware interrupts.
 */
void isr_install(void);

/**
 * Handle an interrupt by processing the stored CPU register state.
 */
void isr_handler(struct cpu_registers *regs);

/**
 * Handle a hardware interrupt (IRQ) by processing the stored CPU register state.
 */
void irq_handler(struct cpu_registers *regs);

#endif // CELESTEOS_ISR_H