/* i8259.c - Functions to interact with the 8259 interrupt controller
 * vim:ts=4 noexpandtab
 */

#include "i8259.h"
#include "lib.h"
#include "idt.h"
#include "paging.h"
#include "x86_desc.h"
#include "syscalls/syscalls.h"

// IRQ linker functions
extern void do_irq_0();
extern void do_irq_1();
extern void do_irq_2();
extern void do_irq_3();
extern void do_irq_4();
extern void do_irq_5();
extern void do_irq_6();
extern void do_irq_7();
extern void do_irq_8();
extern void do_irq_9();
extern void do_irq_10();
extern void do_irq_11();
extern void do_irq_12();
extern void do_irq_13();
extern void do_irq_14();
extern void do_irq_15();

/* Interrupt masks to determine which interrupts are enabled and disabled */
uint8_t master_mask; /* IRQs 0-7  */
uint8_t slave_mask;  /* IRQs 8-15 */


// Data structure containing handlers
uint32_t i8259_handlers[PIC_SIZE*2][MAX_CHAIN] = {{0x0}};
// Table containing IRQ linker functions
void* do_irq_table[16] = {&do_irq_0, &do_irq_1, &do_irq_2, &do_irq_3, &do_irq_4, &do_irq_5, &do_irq_6, &do_irq_7, 
							&do_irq_8, &do_irq_9, &do_irq_10, &do_irq_11, &do_irq_12, &do_irq_13, &do_irq_14, &do_irq_15};

/*
 * i8259_init
 *	  DECRIPTION: Initialize the 8259 PIC
 *	  INPUTS: None
 *	  OUTPUTS: None
 *	  SIDE EFFECTS: Initializes the PIC
 */
void i8259_init(void) {
  uint32_t flags;
  uint8_t mask_m, mask_s;

  cli_and_save(flags);

  // Cache mask values before overwriting them
  mask_m = inb(MASTER_8259_PORT+1);
  mask_s = inb(SLAVE_8259_PORT+1);

  outb(0xFF, MASTER_8259_PORT+1); /* mask all IRQs */
  outb(0xFF, SLAVE_8259_PORT+1);

  /* Execute the master init control sequence */
  outb(ICW1, MASTER_8259_PORT);
  outb(ICW2_MASTER, MASTER_8259_PORT+1);
  outb(ICW3_MASTER, MASTER_8259_PORT+1); // Slave is on IRQ 3
  outb(ICW4, MASTER_8259_PORT+1);

  /* Execute the slave init control sequence */
  outb(ICW1, SLAVE_8259_PORT);
  outb(ICW2_SLAVE, SLAVE_8259_PORT+1);
  outb(ICW3_SLAVE, SLAVE_8259_PORT+1);
  outb(ICW4, SLAVE_8259_PORT+1);

  // Re-enable the interrupts we masked
  outb(mask_m, MASTER_8259_PORT+1);
  outb(mask_m, SLAVE_8259_PORT+1);

 restore_flags(flags);
}

/*
 * register_interrupt_handler
 *	  DESCRIPTION: Installs a new interrupt handler to given IRQ
 *	  INPUTS: irq_num -- IRQ to register handler for
 *			  handler -- new handler function
 *	  OUTPUTS: None
 *	  SIDE EFFECTS: Registers a new IRQ handler
 */
void register_interrupt_handler(uint32_t irq_num, void* handler) {
  uint32_t flags, i;

  cli_and_save(flags);

  send_eoi(irq_num); // TODO in case??
  // (1) Add handler to IRQ list
  for(i = 0; i8259_handlers[irq_num][i] != 0; ++i);
  i8259_handlers[irq_num][i] = (uint32_t)handler;

  // (2) write function to IDT
  install_irq(do_irq_table[irq_num], irq_num);

  // (3) unmask interrupt
  enable_irq(irq_num);

  restore_flags(flags);
}

void remove_interrupt_handler(uint32_t irq_num, void* handler) {
  uint32_t i;

  for(i = 0; i < MAX_CHAIN; ++i) {
	if(i8259_handlers[irq_num][i]) i8259_handlers[irq_num][i] = 0;
	return;
  }

  disable_irq(irq_num);
}


/*
 * enable_irq
 *	  DESCRIPTION: Enable (unmask) the specified IRQ
 *	  INPUTS: irq_num -- IRQ to unmask
 *	  OUTPUTS: None
 *	  SIDE EFFECTS: Unmasks IRQ at irq_num
 */
void enable_irq(uint32_t irq_num) {
  uint16_t port;
  uint8_t value;

  // Need to determine if IRQ is on master or slave
  if(irq_num < PIC_SIZE) port = MASTER_8259_PORT+1;
  else {
	port = SLAVE_8259_PORT+1;
	irq_num -= PIC_SIZE;
  }
  value = inb(port) & ~(1 << irq_num); // Read current mask to avoid issues
  outb(value, port);
}

/*
 * disable_irq
 *	  DESCRIPTION: Disable (mask) the specified IRQ
 *	  INPUTS: irq_num -- IRQ to mask
 *	  OUTPUTS: None
 *	  SIDE EFFECTS: Masks IRQ at irq_num
 */
void disable_irq(uint32_t irq_num) {
  uint16_t port;
  uint8_t value;

  // Need to determine if IRQ is on master or slave
  if(irq_num < PIC_SIZE) port = MASTER_8259_PORT+1;
  else {
	port = SLAVE_8259_PORT+1;
	irq_num -= PIC_SIZE;
  }
  value = inb(port) | (1 << irq_num); // Read current mask to avoid issues
  outb(value, port);
}

/* i8259_mask_all
 *	  DESCRIPTION: Masks all interrupts (for use on startup)
 *	  INPUTS: None
 *	  OUTPUTS: None
 *	  SIDE EFFECTS: Masks all PIC interrupts
 */
void i8259_mask_all() {
  outb(0xFB, MASTER_8259_PORT+1); /* mask all IRQs, except slave connection */
  outb(0xFF, SLAVE_8259_PORT+1);
}

/*
 * send_eoi
 *	  DESCRIPTION: Sends end-of-interrupt signal for the specified IRQ
 *	  INPUTS: irq_num -- IRQ to recieve EOI
 *	  OUTPUTS: None
 *	  SIDE EFFECTS: Device at irq_num recieves EOI
 */
void send_eoi(uint32_t irq_num) {
  if(irq_num >= PIC_SIZE) outb(EOI, SLAVE_8259_PORT);
  outb(EOI, MASTER_8259_PORT);
}

/*
 * do_irq
 *	  DESCRIPTION: Perform handler actions associated with given IRQ
 *	  INPUTS: irq_num -- IRQ who raised interrupt
 *	  OUTPUTS: None
 *	  SIDE EFFECTS: Executes handlers
 */
void do_irq(uint32_t irq_num) {
  uint8_t i;

  for(i = 0; i < MAX_CHAIN; ++i) {
	if(!i8259_handlers[irq_num][i]) continue; // Don't execute null entries
	((void(*)(void))(i8259_handlers[irq_num][i]))(); // Execute function at specified addr
  }

  send_eoi(irq_num); // Send EOI

}
