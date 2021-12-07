/* filesystem_structs.h - Defines filesystem structs used in file and directory operations 
 * vim:ts=4 noexpandtab
 */

#include "../lib.h"

#ifndef FILESYSTEM_STRUCTS_H
#define FILESYSTEM_STRUCTS_H

#define NUM_DATA_BLOCK_ADDR 1023
#define DENTRY_SIZE 64
#define MAX_FILENAME_LENGTH 32

/* Disk block size */
#define BLOCK_SIZE FOUR_KB

/* Maximum files limited by boot block size */
#define MAX_FILES (BLOCK_SIZE / DENTRY_SIZE - 1)

/*** Block formats  ***/

/* directory structure */
// TODO for writable filesystem (just use boot block as directory until then)

/* directory entry structure */
typedef struct __attribute__((packed, aligned(DENTRY_SIZE))) dentry {
  uint8_t filename[MAX_FILENAME_LENGTH];
  uint32_t file_type;
  uint32_t inode_num;
  uint8_t reserved[24]; // not used
} dentry_t;

/* data_block structure */
typedef struct __attribute__((packed, aligned(FOUR_KB))) data_block {
  uint8_t data[FOUR_KB];
} data_block_t;

/* inode structure */
typedef struct __attribute__((packed, aligned(FOUR_KB))) inode {
  uint32_t length;
  uint32_t data_blocks[NUM_DATA_BLOCK_ADDR];
} inode_t;

/* boot block structure */
typedef struct __attribute__((packed, aligned(FOUR_KB))) boot_block {
  uint32_t num_dir_entries;			/* number of directory entries in root */
  uint32_t num_inodes;				/* number of inodes used by the system */
  uint32_t num_data_blocks;			/* number of data blocks used by the filesystem */
  uint8_t reserved[52]; 			/* not used, size is 64 - 3 * sizeof(uint32_t) */
  dentry_t dentries[MAX_FILES]; 	/* maximum number of dentries that will fit into the block */
} boot_block_t;

/* Checks that a given file type is a valid file type */
uint8_t check_valid_file_type(uint32_t file_type);


/*** Exposed file operation and descriptor types ***/

/* Function types for syscall file operation functions */
typedef int32_t(*read_t)(int32_t, void*, int32_t);			/* int32_t read (int32_t fd, void* buf, int32_t nbytes) */
typedef int32_t(*write_t)(int32_t, const void*, int32_t);	/* int32_t write (int32_t fd, const void* buf, int32_t nbytes) */
typedef int32_t(*open_t)(const uint8_t*);					/* int32_t open (const uint8_t* filename) */
typedef int32_t(*close_t)(int32_t);							/* int32_t close (int32_t fd) */

/* filesystem operations table */
typedef struct file_ops_t {
  read_t read;
  write_t write;
  open_t open;
  close_t close;
} file_ops_t;

/* file descriptor structure */
typedef struct __attribute__((packed, aligned(16))) fd { // fd are 4B * 4 = 16B items
  file_ops_t* fops_table;	/* file operations table for this file descriptor */
  uint32_t inode_num;		/* inode number for regular file */
  uint32_t file_pos; 		/* virutal addr within file (0 to length in bytes for regular files) */
  uint32_t flags;			/* not used for regular files */
} fd_t;

#endif /* FILESYSTEM_STRUCTS_H */
