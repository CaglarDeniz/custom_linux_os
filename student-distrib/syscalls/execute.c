/* execute.c - Implements the execute() syscall
 * vim:ts=4 noexpandtab
 */

#include "syscalls.h"
#include "../paging.h"
#include "../loader.h"
#include "../filesystem/filesystem.h"
#include "../x86_desc.h"			/* For tss, USER_DS, USER_CS */
#include "../tasks/tasks.h"

#define USER_MEM_END (FOUR_MB * (USER_MEM_PAGE_INDEX + 1))
#define FN_BUF_SIZE 33

/* Pushes IRET context and IRET-s to program */
#define execute_program(user_ds, esp, cs, eip)	\
do {                                    \
	asm volatile ("                   \n\
            pushl %0				  \n\
            pushl %1				  \n\
            pushfl					  \n\
            pushl %2				  \n\
            pushl %3				  \n\
            iret					  \n\
            "                           \
			:							\
            :"g"(user_ds), "g"(esp), "g"(cs), "g"(eip) \
            : "memory", "cc"            \
    );  								\
} while (0)

/* int32_t execute(const uint8_t* command);
 * Inputs: command - string containing command to be executed 
 *			(executable file and arguments)
 * Return Value: 0 for success, -1 (SYSCALL_ERROR) for failure
 * Function: Creates a new user process which executes the given executable
 */
int32_t execute(const uint8_t* command) {
	uint32_t user_memory_block, pcb_addr, i, b, flags;
	uint8_t save_command[129], filename[FN_BUF_SIZE];

	/* Extract executable file name */
	for(b = 0; command[b] == ' '; b++);
	for(i = b; command[i] && command[i] != ' ' && i < FN_BUF_SIZE; i++) {
		filename[i-b] = command[i];
	}
	filename[i-b] = '\0';

	strcpy((char*) save_command, (char*) command+b);
    
    /* Verify file is an executable */
    if(!is_executable_file(filename)) return SYSCALL_ERROR;
    
    /* Allocate a page for the Program in User Space */
    user_memory_block = add_user_page();
    
    /* Allocate new PCB and init file descriptor table */
    pcb_addr = push_pcb();

    if (pcb_addr == -1 /* Maximum processes reached */ || 
    	load_program(filename) /* Load user program failed */) { 
        // (3) restore paging
        if(current_pcb != (pcb_t*)KERNEL_MEM_END) set_user_page(current_pcb->user_physical_mem_block_num);

        // (4) free user page
        free_user_page(user_memory_block);

        return SYSCALL_ERROR;
    }
    
	strcpy((char*) current_pcb->command, (char*) save_command);

    /* Set the new kernel stack pointer to point to current 8 kB block*/
    update_tss();
    
    /* Track the User memory block used by the current process */
	current_pcb->user_physical_mem_block_num = user_memory_block; 

    /* Set up new context */
    current_pcb->context = (hw_context_t*)(tss.esp0 - sizeof(hw_context_t));

    /* Get eflags */
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
    if (current_tasks[current_task] == current_pcb->parent) {
        current_tasks[current_task] = current_pcb;
    }

    return 0;
}
