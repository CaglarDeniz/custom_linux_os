/* filesystem_driver.c - Initializes the file system 
 * vim:ts=4 noexpandtab
 */

#include "filesystem.h"
#include "filesystem_structs.h"

/* void filesystem_init(unsigned int base_addr);
 * Inputs: base_addr - base address of physical memory address of filesystem image
 * Return Value: none
 * Function: Initializes base addresses used in file and directory operations */
void filesystem_init(unsigned int base_addr) {
    /* Load the boot block */
    root = *((boot_block_t*)base_addr);

    /* Initialize where the inodes are found */
    inode_base = (inode_t*)(base_addr + BLOCK_SIZE);

	/* Initialize the base address for where to find data blocks */
    data_base = (data_block_t*)(inode_base + root.num_inodes);
    
    /* Set the current file descriptor table to one statically allocated in the kernel 
    	for file accesses while in the kernel */
    fd_table = (fd_t*) kernel_fd_table;
}
