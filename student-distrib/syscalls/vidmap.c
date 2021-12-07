/* vidmap.c - Implements the vidmap() syscall
 * vim:ts=4 noexpandtab
 */

#include "syscalls.h"
#include "../paging.h"

/* int32_t vidmap(uint8_t ** screen_start);
 * Inputs: screen_start - pointer in user memory 
 *			that points to the base address of mapped video memory
 * Return Value: 0 on success, 
 *		-1 (SYSCALL_ERROR) for failure
 * Function: Provides a pointer to a virtual address mapped to video memory
 */
int32_t vidmap(uint8_t ** screen_start) {
	/* Verify address to overwrite is in User Memory */
	if (!is_in_user_mem((uint32_t) (screen_start))) 
		return SYSCALL_ERROR;
	
	/* Set the address to the start of vidmap memory */
	*screen_start = (uint8_t*) enable_vidmap();
    return 0;
}
