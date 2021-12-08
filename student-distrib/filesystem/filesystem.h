/* filesystem.h - Defines filesystem functions to be exported and used by other modules 
 * vim:ts=4 noexpandtab
 */

#include "filesystem_structs.h"

#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#define MAX_OPEN_FILES 8

/* File descriptors for STDIN, STDOUT */
#define STDIN 0
#define STDOUT 1

/* File type definitions */
#define FILE_TYPE_RTC 0
#define FILE_TYPE_DIR 1
#define FILE_TYPE_REGULAR 2
/* Note: these are used in check_valid_file_type */

inode_t* inode_base;		/* Physical addresss of first inode */
data_block_t* data_base; 	/* Physical address of first data block */ // hahahahahahaha (name)
fd_t* fd_table;				/* File descriptor table for tracking open files, is of size MAX_OPEN_FILES */
fd_t kernel_fd_table[MAX_OPEN_FILES]; /* Block of memory allocated for file descriptors in the kernel 
											NOTE: on filesystem init, MUST set fd_table to this */

/* Boot block, which also acts as our root directory */
boot_block_t root;

/* Bitmaps for filesystem contents */
unsigned long long inode_bitmap;
uint8_t data_block_bitmap[NUM_DATA_BLOCK_ADDR];

/* Initializes the filesystem using base_addr as the base 
 * 	physical address of the filesystem image in memory */
void filesystem_init(unsigned int base_addr);


/* Obtains file directory entry for a file in current directory 
 *	(TODO currently only root) given a file name */
int32_t read_dentry_by_name(const uint8_t* fname, dentry_t* dentry);

/* Obtains file directory entry for a file in current directory 
 *	(TODO currently only root) given an file d-entry index */
int32_t read_dentry_by_index(uint32_t index, dentry_t* dentry);

/* Reads a number of bytes from a file given an inode and an offset */
int32_t read_data(uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length);

/*** File Operations Tables ***/

/* File operations for regular files */
int32_t file_open(const uint8_t* filename);							/* Opens a file */
int32_t file_close(int32_t fd);										/* Closes a file */
int32_t file_write(int32_t fd, const void* buf, int32_t nbytes);	/* Write a string to a file (TODO does nothing) */
int32_t file_read(int32_t fd, void* buf, int32_t nbytes);			/* Reads a string from a file */

/* File operations table for regular files */
extern file_ops_t file_ops_regular;

/* File operations for directories */
int32_t directory_open(const uint8_t* filename);						/* Opens a directory */
int32_t directory_close(int32_t fd);									/* Closes a directory */
int32_t directory_write(int32_t fd, const void* buf, int32_t nbytes);	/* Writes to a directory (TODO does nothing) */
int32_t directory_read(int32_t fd, void* buf, int32_t nbytes);			/* Reads a file name from a directory */

int32_t new_dentry(const uint8_t* fname); // write to directory
int32_t remove_dentry(const uint8_t* fname); // write to directory

/* File operations table for directories */
extern file_ops_t file_ops_dir;

#endif /* FILESYSTEM_H */
