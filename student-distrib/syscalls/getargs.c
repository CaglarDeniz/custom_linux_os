/* getargs.c - Implements the getargs() syscall
 * vim:ts=4 noexpandtab
 */

#include "syscalls.h"
#include "../lib.h"
#include "../tty.h" /* For MAX_TERMINAL_BUF_SIZE */

/* int32_t getargs(uint8_t* buf, int32_t nbytes);
 * Inputs: buf - buffer to copy the arguments of the command into
 *			nbytes - limit on the number of bytes to copy
 * Return Value: 0 on success, 
 *		-1 (SYSCALL_ERROR) for failure
 * Function: Copies the arguments a user program was run with 
 *			into the given buffer
 */
int32_t getargs(uint8_t* buf, int32_t nbytes) { 
  int32_t i;
  uint8_t* args;

  /* Skip over the first word (executable file name) */
  for(i = 0; i < MAX_TERMINAL_BUF_SIZE; ++i) {
    if(!((current_pcb->command)[i]) || (current_pcb->command[i]) == ' ') {
      break;
    }
  }
  for(; current_pcb->command[i] == ' '; ++i); // skip indermediate spaces

  /* Args start after the executable file name */
  args = (current_pcb->command) + i;
  
  /* Copy the args into the buffer */ 
  strncpy((char*) buf, (char*) args, nbytes);

  if (buf[0] == '\0') return -1; // no args

  return 0;
}

