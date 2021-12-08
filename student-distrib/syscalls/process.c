/* process.c - Defines functions used to set up user processes
 * vim:ts=4 noexpandtab
 */

#include "syscalls_structs.h"
#include "syscalls.h"
#include "../filesystem/filesystem.h"
#include "../paging.h" 				/* For KERNEL_MEM_END, disable_vidmap() */
#include "../tty.h"					/* For STDIN/STDOUT */
#include "../x86_desc.h"			/* For tss */
#include "../i8259.h"               /* For do_irq */
#include "../idt.h" 				/* For exception_handlers */

/* Mask to round address down to an 8 kB when AND */
#define PCB_ADDR_MASK 0xFFFFE000

/* Pointer to the process control block for the current process */
pcb_t *current_pcb = (pcb_t*) KERNEL_MEM_END;

/* Bitmap of available 8 kB blocks with LSB being block ending at 8MB
 * Note that a 1 means a position is available.
 */
uint32_t pcb_bitmap = 0xFFFFFFFF; 

/* Pointer to context for returning to while loop in entry */
hw_context_t *global_context;


/* void setup_fdtable(fd_t* fd_table) 
 * Inputs: fd_table - pointer to file descriptor table to be initalized
 * Return Value: None
 * Function: Initializes the fd table with STDIN and STDOUT
 */
void setup_fdtable(fd_t* fd_table) {
    fd_table[STDIN].fops_table = &file_ops_tty;
    fd_table[STDIN].file_pos = 0;
    fd_table[STDIN].inode_num = 0;
    fd_table[STDIN].flags = 0; // TODO determine flag format for file descriptors
    fd_table[STDOUT].fops_table = &file_ops_tty;
    fd_table[STDOUT].file_pos = 0;
    fd_table[STDOUT].inode_num = 0;
    fd_table[STDOUT].flags = 0; // TODO
}


/* uint32_t is_in_user_mem(uint32_t addr) 
 * Inputs: addr - address to check if it's in the current process' user memory
 * Return Value: 1 for within user memory, 0 else
 * Function: Check if it's in the current process' user memory
 */
uint32_t is_in_user_mem(uint32_t addr) {
    return (addr >= USER_MEM_PAGE_INDEX * FOUR_MB
    	&& addr < (USER_MEM_PAGE_INDEX + 1) * FOUR_MB);
}


/* uint32_t push_pcb(void)
 * Inputs: None
 * Return Value: address of pointer to new pcb in memory, 
 *		-1 (SYSCALL_ERROR) for failure
 * Function: Initalizes a new pcb at the next available location 
 *				at the bottom of kernel memory and updates the 
 *				current pcb along with esp
 * NOTE: current_pcb MUST be populated by the caller!
 */
uint32_t push_pcb(void) {
	/* NOTE: because this function messes with ESP, do NOT allocate local vars */
	static uint32_t next_pid = 0; /* Start pid-s at 0 */
	
	/* Temporary varaible for new pcb */
	pcb_t *new_pcb;
	uint32_t pcb_num = 0, pcb_bitmap_temp; /* 0 is PCB ending at KERNEL_MEM_END, 
								1 is 8 kB higher on stack / lower addr */
	
	/* Check if the first open pcb slot is past the process maximum */
	if((pcb_bitmap & -pcb_bitmap) >= (1 << MAX_PROCESSES)) {
		return -1; /* Maximum processes reached: Cannot allocate PCB for another process */
	}
	
	/* pcb_num = log2(first avail bit) */
	pcb_bitmap_temp = (pcb_bitmap & -pcb_bitmap);
	while(pcb_bitmap_temp > 1) {
		pcb_num++;
		pcb_bitmap_temp >>= 1;
	}
	
	/* Mark block as in use */
	pcb_bitmap &= ~(pcb_bitmap & -pcb_bitmap);
	
	/* Set the new pcb to be (8 MB - 8 kB * (pcb_num + 1)) to (8 MB - 8 kB * pcb_num) */
	new_pcb = (pcb_t*) ((void*) KERNEL_MEM_END - (PROCESS_STACK_SIZE * (pcb_num + 2)));
	
	/* Clear pcb (potentially leftover data from before */
	memset(new_pcb, 0, sizeof(pcb_t)); 
	
	new_pcb->pcb_num = pcb_num;
	new_pcb->pid = next_pid++;
	new_pcb->parent = new_pcb->child = (pcb_t*) KERNEL_MEM_END; /* End of kernel memory indicates no process */
	
	setup_fdtable((fd_t*)(new_pcb->process_fd_table));
	
	/* Clear current process' command for get_args to use later */
	memset(new_pcb->command, 0, sizeof(new_pcb->command));

	if(current_pcb != (pcb_t*) KERNEL_MEM_END && current_pcb != NULL) {
		/* If we already have a pcb, then modify it */
		new_pcb->parent = current_pcb;
		current_pcb->child = new_pcb;
	}
	
	current_pcb = new_pcb;
	/* Set current fd_table to be this process' fd_table */
	fd_table = (fd_t*) current_pcb->process_fd_table;
	
	return (uint32_t) current_pcb;
}


/* uint32_t pop_pcb(void)
 * Inputs: None
 * Return Value: address of pointer to popped pcb in memory, 
 *		-1 (SYSCALL_ERROR) for failure
 * Function: De-initalizes the current pcb and reverts state to parent pcb
 * NOTE: current_pcb MUST be de-populated by the caller!
 */
uint32_t pop_pcb(void) {
	if((uint32_t) current_pcb >= KERNEL_MEM_END) {
		return -1; /* No current process, no pcb to pop */
	}
	
	/* Mark current pcb as not in use */
	pcb_bitmap |= (1 << current_pcb->pcb_num);
	
	/* Unpage vidmap */
	//disable_vidmap();
	// TODO  store vidmap status in pcb
	
	pcb_t *popped = current_pcb;
	current_pcb = current_pcb->parent;
	return (uint32_t) popped; 
}

/* uint32_t update_tss(void)
 * Inputs: none
 * Return Value: none
 * Function: Updates tss.esp0 based on current_pcb 
 */
void update_tss(void) {
	tss.esp0 = KERNEL_MEM_END - (PROCESS_STACK_SIZE * (current_pcb->pcb_num+1));
}

/* void do_irq_main(hw_context_t *context);
 * Inputs: context - new context to swap to
 * Return Value: esp to go to
 * Function: Finishes setting up context and calls irqs
 */
hw_context_t* do_irq_main(hw_context_t *context) {
	// make context point to previous context
	if ((uint32_t) current_pcb >= KERNEL_MEM_END) { // no current process
		context->parent = global_context;
		global_context = context;
	} else {
		context->parent = current_pcb->context;
		current_pcb->context = context;
	}

	if (context->irq_num < 22) {
		// exception
		((void(*)(hw_context_t*))exception_handlers[context->irq_num])(context);
	} else if (context->irq_num == 0x80) {
		// syscall
		if ((context->eax < 1) || (context->eax > 14)) { // TODO signals
			context->eax = -1; // return -1
		} else {
			context->eax = syscall_shim(context->ebx, context->ecx, context->edx, context->eax);
		}
	} else if ((32 <= context->irq_num) && (context->irq_num < 48)) {
		// irq
		do_irq(context->irq_num - 32);
	}

	if ((uint32_t) current_pcb >= KERNEL_MEM_END) { // no current process
		return global_context;
	} else {
		return current_pcb->context;
	}
}

/* void swap_context(hw_context_t *context)
 * Inputs: context - old context to swap back to
 * Return Value: none
 * Function: Finishes setting up context and calls irqs
 */
void swap_context(hw_context_t *context) {
	// make context point to previous context
	if ((uint32_t) current_pcb >= KERNEL_MEM_END) { // no current process
		global_context = context;
	} else {
		current_pcb->context = context;
	}
}
