/* creat.c - Implements the creat() syscall
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
int32_t creat(const uint8_t* filename) {

// if it exists just open it
  dentry_t file_dentry;
  if(!read_dentry_by_name(filename, &file_dentry)) return open(filename);

// create then open
  new_dentry(filename);
  return open(filename);
}

