#ifndef CELESTEOS_PIC_H
#define CELESTEOS_PIC_H

#include <stdint.h>

#define PIC1         0x20 /* Master PIC base address */
#define PIC2         0xA0 /* Slave PIC base address */
#define PIC1_COMMAND PIC1
#define PIC1_DATA    (PIC1 + 1)
#define PIC2_COMMAND PIC2
#define PIC2_DATA    (PIC2 + 1)
#define PIC_EOI      0x20 /* End-of-interrupt */

#define ICW1_ICW4      0x01 /* ICW4 (not) needed */
#define ICW1_SINGLE    0x02 /* Single (cascade) mode */
#define ICW1_INTERVAL4 0x04 /* Call address interval 4 (8) */
#define ICW1_LEVEL     0x08 /* Level triggered (edge) mode */
#define ICW1_INIT      0x10 /* Initialization - required! */

#define ICW4_8086       0x01 /* 8086/88 (MCS-80/85) mode */
#define ICW4_AUTO       0x02 /* Auto (normal) EOI */
#define ICW4_BUF_SLAVE  0x08 /* Buffered mode/slave */
#define ICW4_BUF_MASTER 0x0C /* Buffered mode/master */
#define ICW4_SFNM       0x10 /* Special fully nested mode */

/**
 * Remap the PIC to the specified interrupt vector offsets.
 */
void pic_remap(int offset1, int offset2);

/**
 * Send an EOI to the PIC for the specified IRQ.
 */
void pic_send_eoi(uint8_t irq);

/**
 * Mask (disable) the specified IRQ on the PIC.
 */
void pic_mask_irq(uint8_t irq);

/**
 * Unmask (enable) the specified IRQ on the PIC.
 */
void pic_unmask_irq(uint8_t irq);

#endif // CELESTEOS_PIC_H