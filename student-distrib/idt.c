/* idt.c - Initialization and installation code for IDT 
 *		and exception handlers as well as basic exception handling
 * vim:ts=4 noexpandtab
 */

#include "lib.h"
#include "x86_desc.h"
#include "idt.h"
#include "syscalls/syscalls.h"
#include "tasks/screen.h"

uint32_t exception_handlers[22];

static void divide_by_zero_handler(hw_context_t* context);
static void single_step_handler(hw_context_t* context);
static void nmi_handler(hw_context_t* context);
static void breakpoint_handler(hw_context_t* context);
static void overflow_handler(hw_context_t* context);
static void bound_range_exceeded_handler(hw_context_t* context);
static void invalid_opcode_handler(hw_context_t* context);
static void coprocessor_not_available_handler(hw_context_t* context);
static void double_fault_handler(hw_context_t* context);
static void coprocessor_segment_overrun_handler(hw_context_t* context);
static void invalid_task_state_handler(hw_context_t* context);
static void segment_not_present_handler(hw_context_t* context);
static void stack_segment_fault_handler(hw_context_t* context);
static void general_protection_fault_handler(hw_context_t* context);
static void page_fault_handler(hw_context_t* context);
static void reserved_handler(hw_context_t* context);
static void x87_floating_point_exception_handler(hw_context_t* context);
static void alignment_check_handler(hw_context_t* context);
static void machine_check_handler(hw_context_t* context);
static void simd_floating_point_exception_handler(hw_context_t* context);
static void virtualization_exception_handler(hw_context_t* context);
static void cpe_handler(hw_context_t* context);

extern void syscall_handler(); // defined in syscalls/syscalls.S
extern void do_exc_0(); // defined in irq.S
extern void do_exc_1(); // defined in irq.S
extern void do_exc_2(); // defined in irq.S
extern void do_exc_3(); // defined in irq.S
extern void do_exc_4(); // defined in irq.S
extern void do_exc_5(); // defined in irq.S
extern void do_exc_6(); // defined in irq.S
extern void do_exc_7(); // defined in irq.S
extern void do_exc_8(); // defined in irq.S
extern void do_exc_9(); // defined in irq.S
extern void do_exc_10(); // defined in irq.S
extern void do_exc_11(); // defined in irq.S
extern void do_exc_12(); // defined in irq.S
extern void do_exc_13(); // defined in irq.S
extern void do_exc_14(); // defined in irq.S
extern void do_exc_15(); // defined in irq.S
extern void do_exc_16(); // defined in irq.S
extern void do_exc_17(); // defined in irq.S
extern void do_exc_18(); // defined in irq.S
extern void do_exc_19(); // defined in irq.S
extern void do_exc_20(); // defined in irq.S
extern void do_exc_21(); // defined in irq.S

void idt_init() {
  uint16_t i;

  for(i = 0; i < 256; ++i) {
    SET_IDT_ENTRY(idt[i], 0x0);
    idt[i].seg_selector = KERNEL_CS;
    idt[i].reserved4 = 0x0;
    idt[i].reserved3 = 0x1;
    idt[i].reserved2 = 0x1;
    idt[i].reserved1 = 0x1;
    idt[i].size = 0x1;
    idt[i].dpl = 0x0; // TODO not all
    idt[i].present = 0x0;
 }

  for(i = 0; i < 0x15; ++i) idt[i].present = 0x01;
  idt[0x80].present = 0x01;
  idt[0x80].dpl = 0x03;

  // 0x00 Divide by Zero
  SET_IDT_ENTRY(idt[0], &do_exc_0);
  exception_handlers[0] = (uint32_t)&divide_by_zero_handler;
  // 0x01 Single-Step Interrupt
  SET_IDT_ENTRY(idt[1], &do_exc_1);
  exception_handlers[1] = (uint32_t)&single_step_handler;
  // 0x02 NMI
  SET_IDT_ENTRY(idt[2], &do_exc_2);
  exception_handlers[2] = (uint32_t)&nmi_handler;
  // 0x03 Breakpoint
  SET_IDT_ENTRY(idt[3], &do_exc_3);
  exception_handlers[3] = (uint32_t)&breakpoint_handler;
  // 0x04 Overflow
  SET_IDT_ENTRY(idt[4], &do_exc_4);
  exception_handlers[4] = (uint32_t)&overflow_handler;
  // 0x05 Bound Range Exceeded
  SET_IDT_ENTRY(idt[5], &do_exc_5);
  exception_handlers[5] = (uint32_t)&bound_range_exceeded_handler;
  // 0x06 Invalid Opcode
  SET_IDT_ENTRY(idt[6], &do_exc_6);
  exception_handlers[6] = (uint32_t)&invalid_opcode_handler;
  // 0x07 Coprocessor Not Available
  SET_IDT_ENTRY(idt[7], &do_exc_7);
  exception_handlers[7] = (uint32_t)&coprocessor_not_available_handler;
  // 0x08 Double Fault
  SET_IDT_ENTRY(idt[8], &do_exc_8);
  exception_handlers[8] = (uint32_t)&double_fault_handler;
  // 0x09 Coprocessor Segment Overrun
  SET_IDT_ENTRY(idt[9], &do_exc_9);
  exception_handlers[9] = (uint32_t)&coprocessor_segment_overrun_handler;
  // 0x0A Invalid Task State Segment
  SET_IDT_ENTRY(idt[10], &do_exc_10);
  exception_handlers[10] = (uint32_t)&invalid_task_state_handler;
  // 0x0B Segment Not Present
  SET_IDT_ENTRY(idt[11], &do_exc_11);
  exception_handlers[11] = (uint32_t)&segment_not_present_handler;
  // 0x0C Stack Segment Fault
  SET_IDT_ENTRY(idt[12], &do_exc_12);
  exception_handlers[12] = (uint32_t)&stack_segment_fault_handler;
  // 0x0D General Protection Fault
  SET_IDT_ENTRY(idt[13], &do_exc_13);
  exception_handlers[13] = (uint32_t)&general_protection_fault_handler;
  // 0x0E Page Fault
  SET_IDT_ENTRY(idt[14], &do_exc_14);
  exception_handlers[14] = (uint32_t)&page_fault_handler;
  // 0x0F reserved
  SET_IDT_ENTRY(idt[15], &do_exc_15);
  exception_handlers[15] = (uint32_t)&reserved_handler;
  // 0x10 x87 Floating Point Exception
  SET_IDT_ENTRY(idt[16], &do_exc_16);
  exception_handlers[16] = (uint32_t)&x87_floating_point_exception_handler;
  // 0x11 Alignment Check
  SET_IDT_ENTRY(idt[17], &do_exc_17);
  exception_handlers[17] = (uint32_t)&alignment_check_handler;
  // 0x12 Machine Check
  SET_IDT_ENTRY(idt[18], &do_exc_18);
  exception_handlers[18] = (uint32_t)&machine_check_handler;
  // 0x13 SIMD Floating Point Exception
  SET_IDT_ENTRY(idt[19], &do_exc_19);
  exception_handlers[19] = (uint32_t)&simd_floating_point_exception_handler;
  // 0x14 Virtualization Exception
  SET_IDT_ENTRY(idt[20], &do_exc_20);
  exception_handlers[20] = (uint32_t)&virtualization_exception_handler;
  // 0x15 Control Protection Exception
  SET_IDT_ENTRY(idt[21], &do_exc_21);
  exception_handlers[21] = (uint32_t)&cpe_handler;

  // 0x80 Syscalls
  SET_IDT_ENTRY(idt[0x80], &syscall_handler);
  
}

/* install_irq
 *    DESCRIPTION: Installs the function handler for a PIC interrupt
 *    INPUTS: void* handler -- Address of PIC handler function
 *            int irq_num -- IRQ num on PIC
 *    OUTPUTS: None
 *    SIDE EFFECTS: Installs a new handler to the IDT
 */
void install_irq(void* handler, int irq_num) {
  SET_IDT_ENTRY(idt[0x20+irq_num], handler); // Entries 0x20-0x2F are reserved for the PIC
  idt[0x20+irq_num].present = 0x01;
  idt[0x20+irq_num].dpl = 0x00;
}

/*
 * divide_by_zero_handler
 *    DESCRIPTION: Handler for IRQ entry 0x00
 *    INPUTS: None
 *    OUTPUTS: None
 *    SIDE EFFECTS: Screen of death
 */
static void divide_by_zero_handler(hw_context_t* context) {
  printf("0x00: Divide by Zero Exception");
  halt(0);
}

/*
 * single_step_handler
 *    DESCRIPTION: Handler for IRQ entry 0x01
 *    INPUTS: None
 *    OUTPUTS: None
 *    SIDE EFFECTS: Screen of death
 */
static void single_step_handler(hw_context_t* context) {
  printf("0x01: Single Step Interrupt");
  halt(0);
}

/*
 * nmi_handler
 *    DESCRIPTION: Handler for IRQ entry 0x02
 *    INPUTS: None
 *    OUTPUTS: None
 *    SIDE EFFECTS: Screen of death
 */
static void nmi_handler(hw_context_t* context) {
  printf("0x02: Non Maskable Interrupt");
  halt(0);
}

/*
 * breakpoint_handler
 *    DESCRIPTION: Handler for IRQ entry 0x03
 *    INPUTS: None
 *    OUTPUTS: None
 *    SIDE EFFECTS: Screen of death
 */
static void breakpoint_handler(hw_context_t* context) {
  printf("0x03: Breakpoint Interrupt");
  halt(0);
}

/*
 * overflow_handler
 *    DESCRIPTION: Handler for IRQ entry 0x04
 *    INPUTS: None
 *    OUTPUTS: None
 *    SIDE EFFECTS: Screen of death
 */
static void overflow_handler(hw_context_t* context) {
  printf("0x04: Overflow");
  halt(0);
}

/*
 * bound_range_exceeded_handler
 *    DESCRIPTION: Handler for IRQ entry 0x05
 *    INPUTS: None
 *    OUTPUTS: None
 *    SIDE EFFECTS: Screen of death
 */
static void bound_range_exceeded_handler(hw_context_t* context) {
  printf("0x05: Bound Range Exceeded");
  halt(0);
}

/*
 * invalid_opcode_handler
 *    DESCRIPTION: Handler for IRQ entry 0x06
 *    INPUTS: None
 *    OUTPUTS: None
 *    SIDE EFFECTS: Screen of death
 */
static void invalid_opcode_handler(hw_context_t* context) {
  printf("0x06: Invalid Opcode");
  halt(0);
}

/*
 * coprocessor_not_available_interrupt
 *    DESCRIPTION: Handler for IRQ entry 0x07
 *    INPUTS: None
 *    OUTPUTS: None
 *    SIDE EFFECTS: Screen of death
 */
static void coprocessor_not_available_handler(hw_context_t* context) {
  printf("0x07: Coprocessor Not Available");
  halt(0);
}

/*
 * double_fault_handler
 *    DESCRIPTION: Handler for IRQ entry 0x08
 *    INPUTS: None
 *    OUTPUTS: None
 *    SIDE EFFECTS: Screen of death
 */
static void double_fault_handler(hw_context_t* context) {
  printf("0x08: Double Fault");
  halt(0);
}

/*
 * coprocessor_segment_overrun_handler
 *    DESCRIPTION: Handler for IRQ entry 0x09
 *    INPUTS: None
 *    OUTPUTS: None
 *    SIDE EFFECTS: Screen of death
 */
static void coprocessor_segment_overrun_handler(hw_context_t* context) {
  printf("0x09: Coprocessor Segment Overrun");
  halt(0);
}

/*
 * invalid_task_state_handler
 *    DESCRIPTION: Handler for IRQ entry 0x0A
 *    INPUTS: None
 *    OUTPUTS: None
 *    SIDE EFFECTS: Screen of death
 */
static void invalid_task_state_handler(hw_context_t* context) {
  printf("0x0A: Invalid Task State Segment");
  halt(0);
}

/*
 * segment_not_present_handler
 *  DESCRIPTION: Handler for IRQ entry 0x0B
 *    INPUTS: None
 *    OUTPUTS: None
 *    SIDE EFFECTS: Screen of death
 */
static void segment_not_present_handler(hw_context_t* context) {
  printf("0x0B: Segment Not Present");
  halt(0);
}

/*
 * stack_segment_fault_handler
 *    DESCRIPTION: Handler for IRQ entry 0x0C
 *    INPUTS: None
 *    OUTPUTS: None
 *    SIDE EFFECTS: Screen of death
 */
static void stack_segment_fault_handler(hw_context_t* context) {
  printf("0x0C: Stack Segment Fault");
  halt(0);
}

/*
 * general_protection_fault_handler
 *    DESCRIPTION: Handler for IRQ entry 0x0D
 *    INPUTS: None
 *    OUTPUTS: None
 *    SIDE EFFECTS: Screen of death
 */
static void general_protection_fault_handler(hw_context_t* context) {
  printf("0x0D: General Protection Fault\n");
  halt(0);
}

/*
 * page_fault_handler
 *    DESCRIPTION: Handler for IRQ entry 0x0E
 *    INPUTS: None
 *    OUTPUTS: None
 *    SIDE EFFECTS: Screen of death
 */
static void page_fault_handler(hw_context_t* context) {
  printf("0x0E: Page Fault\n");
  printf("EIP: 0x%x\n", context->ret);
  die("");
  halt(0);
}

/*
 * reserved_handler
 *    DESCRIPTION: Handler for IRQ entry 0x0F
 *    INPUTS: None
 *    OUTPUTS: None
 *    SIDE EFFECTS: Screen of death
 */
static void reserved_handler(hw_context_t* context) {
  printf("0x0F: Reserved Exception");
  halt(0);
}

/*
 * x87_floating_point_exception_handler
 *    DESCRIPTION: Handler for IRQ entry 0x10
 *    INPUTS: None
 *    OUTPUTS: None
 *    SIDE EFFECTS: Screen of death
 */
static void x87_floating_point_exception_handler(hw_context_t* context) {
  printf("0x10: x87 Floating Point Exception");
  halt(0);
}

/*
 * alignment_check_handler
 *    DESCRIPTION: Handler for IRQ entry 0x11
 *    INPUTS: None
 *    OUTPUTS: None
 *    SIDE EFFECTS: Screen of death
 */
static void alignment_check_handler(hw_context_t* context) {
  printf("0x11: Alignment Check");
  halt(0);
}

/*
 * machine_check_handler
 *    DESCRIPTION: Handler for IRQ entry 0x12
 *    INPUTS: None
 *    OUTPUTS: None
 *    SIDE EFFECTS: Screen of death
 */
static void machine_check_handler(hw_context_t* context) {
  printf("0x12: Machine Check");
  halt(0);
}

/*
 * simd_floating_point_handler
 *    DESCRIPTION: Handler for IRQ entry 0x13
 *    INPUTS: None
 *    OUTPUTS: None
 *    SIDE EFFECTS: Screen of death
 */
static void simd_floating_point_exception_handler(hw_context_t* context) {
  printf("0x13: SIMD Floating Point Handler");
  halt(0);
}

/*
 * virtualization_exception_handler
 *    DESCRIPTION: Handler for IRQ entry 0x14
 *    INPUTS: None
 *    OUTPUTS: None
 *    SIDE EFFECTS: Screen of death
 */
static void virtualization_exception_handler(hw_context_t* context) {
  printf("0x14: Virtualization Exception");
  halt(0);
}

/*
 * cpe_handler
 *    DESCRIPTION: Handler for IRQ entry 0x15
 *    INPUTS: None
 *    OUTPUTS: None
 *    SIDE EFFECTS: Screen of death
 */
static void cpe_handler(hw_context_t* context) {
  printf("0x15: Control Protection Exception");
  halt(0);
}
