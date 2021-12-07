/* close.c - Implements the close() syscall
 * vim:ts=4 noexpandtab
 */

#include "syscalls.h"
#include "../filesystem/filesystem.h"

/* int32_t close(int32_t fd);
 * Inputs: fd - file descriptor of file to be closed
 * Return Value: 0 for success, -1 (SYSCALL_ERROR) for failure
 * Function: Clears the file descriptor for a given file
 */
int32_t close(int32_t fd) {
  if ((fd < 0) || (fd > 7)) return SYSCALL_ERROR;
	/* If not closable or not in the file descriptor table */
    if((fd == STDIN) || (fd == STDOUT)) return SYSCALL_ERROR; // Can't close stdin or stdout
    else if(!fd_table[fd].fops_table) return SYSCALL_ERROR;

    /* open will reset all values in file descriptor, so we only need to clear 
     * file_ops ptr because we use it to identify if the fd is populates
     */

	/* Call close() */
    (fd_table[fd].fops_table->close)(fd);
    
    /* Clear the entry in the file descriptor table */
    fd_table[fd].fops_table = 0; 
    return 0;
}

