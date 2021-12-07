/* idt.h - Function headers for Interrupt Descriptor Table
 * vim:ts=4 noexpandtab
 */

#ifndef IDT_H
#define IDT_H

#include "lib.h"

uint8_t PAGE_FAULT_FLAG; /* Used in tests.c for debugging */

/* Initializes IDT */
void idt_init();

/* Installs a handler in the IDT */
void install_irq(void* handler, int irq_num);

extern uint32_t exception_handlers[22];

#endif /* IDT_H */
