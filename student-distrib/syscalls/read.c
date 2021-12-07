/* read.c - Implements the read() syscall
 * vim:ts=4 noexpandtab
 */

#include "syscalls.h"
#include "../filesystem/filesystem.h"

/* int32_t read(int32_t fd, void* buf, int32_t nbytes);
 * Inputs: fd - file descriptor of file to be read
 *			buf - buffer to contain string read from file
 *			nbytes - number of bytes to be read
 * Return Value: number of bytes read
 * Function: Reads from the file by calling the appropriate read function
 */
int32_t read(int32_t fd, void* buf, int32_t nbytes) {
  if ((fd < 0) || (fd > 7)) return SYSCALL_ERROR;
	if(!fd_table[fd].fops_table) return SYSCALL_ERROR;
	return (fd_table[fd].fops_table->read)(fd, buf, nbytes);
}
