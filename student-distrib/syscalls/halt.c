/* halt.c - Implements the halt() syscall
 * vim:ts=4 noexpandtab
 */

#include "syscalls.h"
#include "../paging.h"
#include "../loader.h"
#include "../filesystem/filesystem.h"
#include "../x86_desc.h"			/* For tss, USER_DS, USER_CS */
#include "../tasks/tasks.h"

/* int32_t halt(uint8_t status);
 * Inputs: status - status that the user program terminated with
 * Return Value: 0 for success, -1 (SYSCALL_ERROR) for failure
 * Function: Terminates the current user process
 */
int32_t halt(uint8_t status) {
  uint32_t i;

  // (1) close fds (NOTE: do not close STDIN, STDOUT)
  for(i = 2; i < MAX_OPEN_FILES; ++i) 
	if(fd_table[i].fops_table) (fd_table[i].fops_table->close)(i);

  // (2) restore parent paging
  if(current_pcb->parent != (pcb_t*)KERNEL_MEM_END) set_user_page(current_pcb->parent->user_physical_mem_block_num);

  // (3) update current_tasks
  if (current_tasks[current_task] == current_pcb) {
    current_tasks[current_task] = current_pcb->parent;
  }

  // (4) free user page
  free_user_page(current_pcb->user_physical_mem_block_num);

  // (5) restore parent data
  pop_pcb();

  // (6) set tss and set return value
  if(current_pcb != (pcb_t*)KERNEL_MEM_END) {
    update_tss();
    current_pcb->context->eax = status;
  }

  // (7) set fd table
  fd_table = (fd_t*) current_pcb->process_fd_table;
  
  return 0;
}
