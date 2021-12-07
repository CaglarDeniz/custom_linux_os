/* tasks.c - Functions to switch processes and view screens
 * vim:ts=4 noexpandtab
 */

#include "../syscalls/syscalls.h"
#include "screen.h"
#include "tasks.h"
#include "../loader.h"
#include "../paging.h"
#include "../x86_desc.h"			/* For tss */
#include "../i8259.h"

/* int32_t switch_view_screen(int task);
 * Inputs: task - task number whose screen to display
 * Return Value: 0 for success, -1 for failure
 * Function: Switches screen that is displayed on the terminal
 */
int32_t switch_view_screen(int task_id) {
  change_view_screen(task_id);
  return 0;
}

/* int32_t switch_process(int task_id);
 * Inputs: task_id - task number to switch execution to
 * Return Value: 0 for success, -1 for failure
 * Function: 
 */
int32_t switch_process(int task_id) {
	if(current_task == task_id)
		return 0;

	/* Set current task to new task_id */
	current_task = task_id;
	current_pcb = current_tasks[task_id];
	change_process_screen(task_id);
	if((current_pcb == NULL) || ((uint32_t)current_pcb == KERNEL_MEM_END)) {
		uint32_t user_memory_block, pcb_addr, flags;
		uint8_t* filename = (uint8_t*)"shell";

		/* Verify file is an executable */
		if(!is_executable_file(filename)) return SYSCALL_ERROR; // sadge
		
		/* Allocate a page for the Program in User Space */
		user_memory_block = add_user_page();
		
		/* Allocate new PCB and init file descriptor table */
		pcb_addr = push_pcb();

		if (pcb_addr == -1 /* Maximum processes reached */ || 
			load_program(filename) /* Load user program failed */) { 

			/* This code should never run */
			/* This will only ever run if we're trying to initialize more terminals than there are pcbs */

			free_user_page(user_memory_block);

			return SYSCALL_ERROR;
		}

		strcpy((char*) current_pcb->command, (char*) filename);
		
		/* Track the User memory block used by the current process */
		current_pcb->user_physical_mem_block_num = user_memory_block; 

		/* Set the new kernel stack pointer to point to current 8 kB block*/
	    update_tss();

	    /* Set up new context */
	    current_pcb->context = (hw_context_t*)(tss.esp0 - sizeof(hw_context_t));

	    asm volatile ("             \n\
	            pushfl              \n\
	            popl %0             \n\
	            "
	            : "=r"(flags)
	            :
	            : "memory"
	    );

	    /* Push IRET Context */
	    current_pcb->context->ss = USER_DS;
	    current_pcb->context->esp = USER_MEM_END;
	    current_pcb->context->eflags = flags;
	    current_pcb->context->cs = USER_CS;
	    current_pcb->context->ret = (uint32_t)get_start();

	    /* Update current_tasks */
		current_tasks[task_id] = current_pcb;
	}
	else {
		// restore process' paging
		set_user_page(current_pcb->user_physical_mem_block_num);

		/* Set the new kernel stack pointer to point to current 8 kB block*/
		update_tss();

		/* Update fd_table */
		fd_table = (fd_t*) current_pcb->process_fd_table;
	}
	
	return 0;
}
