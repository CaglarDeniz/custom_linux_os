/* open.c - Implements the open() syscall
 * vim:ts=4 noexpandtab
 */

#include "syscalls.h"
#include "../filesystem/filesystem.h"
#include "../devices/devices.h"

/* int32_t open(const uint8_t* filename);
 * Inputs: filename - name of file to be opened
 * Return Value: file descriptor for opened file on success, 
 *		-1 (SYSCALL_ERROR) for failure
 * Function: Opens the file by calling its open function
 */
int32_t open(const uint8_t* filename) {
    uint32_t fd, found_empty_file_descriptor;
    dentry_t file_dentry;

	/* If not successful (file dir-entry doesn't exist in root), return error */
    if(read_dentry_by_name(filename, &file_dentry)) {
    	return SYSCALL_ERROR;
    }

    /* Find next available file descriptor, return error if not available */
    found_empty_file_descriptor = 0;
    for(fd = 2; fd < MAX_OPEN_FILES; ++fd) {
    	if(!fd_table[fd].fops_table) {
    		found_empty_file_descriptor = 1;
    		break;
    	}
    }
    if(!found_empty_file_descriptor) {
    	return SYSCALL_ERROR;
    }

	/* Set default values for an opened file descriptor */
    fd_table[fd].inode_num = file_dentry.inode_num;
    fd_table[fd].file_pos = 0;
    fd_table[fd].flags = 0; // We're not using these

	/* Set file operations based on file type */
	switch(file_dentry.file_type) {
		case FILE_TYPE_REGULAR:
			fd_table[fd].fops_table = &file_ops_regular; // ptr to file operations
			break;
		case FILE_TYPE_DIR:
			fd_table[fd].fops_table = &file_ops_dir;
			break;
		case FILE_TYPE_RTC:
			fd_table[fd].fops_table = &file_ops_rtc;
			if((fd_table[fd].fops_table->open)((const uint8_t*)fd) == SYSCALL_ERROR) return SYSCALL_ERROR;
			return fd;
			break;
		default:
			return -1; // Unsupported file type
			break;
	}

	/* Open file */
    if((fd_table[fd].fops_table->open)(file_dentry.filename) == SYSCALL_ERROR) return SYSCALL_ERROR;
    return fd;
}
