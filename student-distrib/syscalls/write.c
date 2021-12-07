/* write.c - Implements the write() syscall
 * vim:ts=4 noexpandtab
 */

#include "syscalls.h"
#include "../filesystem/filesystem.h"

/* int32_t write(int32_t fd, const void* buf, int32_t nbytes);
 * Inputs: fd - file descriptor of file to be written
 *			buf - buffer to contain string to write to file
 *			nbytes - number of bytes to be written
 * Return Value: 0 for success, -1 for failure
 * Function: Writes to the file by calling the appropriate write function
 */
int32_t write(int32_t fd, const void* buf, int32_t nbytes) {
  if ((fd < 0) || (fd > 7)) return SYSCALL_ERROR;
  if(!fd_table[fd].fops_table) return SYSCALL_ERROR;
  return (fd_table[fd].fops_table->write)(fd, buf, nbytes);
}
