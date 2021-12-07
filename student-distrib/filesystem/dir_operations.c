/* dir_operations.c - Implements file operations for directories 
 *						and operatiosn for obtaining directory entires 
 * vim:ts=4 noexpandtab
 */

#include "filesystem.h"

/*** File Operations for Directories ***/

file_ops_t file_ops_dir = {directory_read, directory_write, directory_open, directory_close};

/* int32_t directory_open(const uint8_t* path);
 * Inputs: path - name of directory to be opened
 * Return Value: 0 for success, -1 for failure
 * Function: Does nothing 
 */
int32_t directory_open(const uint8_t* path) {
    // TODO change current directory
    return 0;
}

/* int32_t directory_close(int32_t fd);
 * Inputs: fd - file descriptor of directory to close
 * Return Value: 0 for success, -1 for failure
 * Function: Does nothing 
 */
int32_t directory_close(int32_t fd) {
    return 0;
}

/* int32_t directory_read(int32_t fd, void* buf, int32_t nbytes);
 * Inputs: fd - file descriptor of directory to read from
 *			buf - buffer for filename to be read 
 *			nbytes - number of bytes to read
 * Return Value: number of bytes read
 * Function: Reads the next filename from the directory 
 */
int32_t directory_read(int32_t fd, void* buf, int32_t nbytes) {
  if (fd_table[fd].file_pos == root.num_inodes) return -1;
  if(nbytes > MAX_FILENAME_LENGTH) nbytes = MAX_FILENAME_LENGTH;
  int32_t n = strlcpy(buf, (const int8_t*)root.dentries[fd_table[fd].file_pos].filename, nbytes);
  strncpy((char*)buf, (char*)root.dentries[fd_table[fd].file_pos].filename, nbytes);

  fd_table[fd].file_pos += 0x01;

  return n; // return # of bytes read
}

/* int32_t directory_write(int32_t fd, const void* buf, int32_t nbytes);
 * Inputs: fd - file descriptor of directory to write to
 *			buf - buffer of filename to create in the directory
 *			nbytes - number of bytes in the filename to write
 * Return Value: 0 for success, -1 for failure
 * Function: Does nothing, always fails since it is not supported
 */
int32_t directory_write(int32_t fd, const void* buf, int32_t nbytes) {
    return -1; 
}

/* int32_t read_dentry_by_name(const uint8_t* fname, dentry_t* dentry);
 * Inputs: fname - name of file whose dentry we want
 *			dentry - pointer to dir-entry that will be filled by 
 *					the dir-entry of the input file
 * Return Value: 0 for success, -1 for failure
 * Function: Loops through the root directory and search for file with 
 *				file name 'fname'
 */
int32_t read_dentry_by_name(const uint8_t* fname, dentry_t* dentry) {
    int32_t i;
    // don't match empty filename
    if (fname[0]=='\0') return -1;

	/* Iterate through all dentries in root directory */
    for(i = 0; i < MAX_FILES; ++i) {
    	/* Directory entry does not exist */
        if(!check_valid_file_type(root.dentries[i].file_type)) 
        	continue;
        
        /* File name matches */
        if(!strncmp((char*)fname, (char*)root.dentries[i].filename, MAX_FILENAME_LENGTH)
                && strlen((char*)fname) <= MAX_FILENAME_LENGTH) {
            memcpy(dentry, &(root.dentries[i]), DENTRY_SIZE);
            return 0;
        }
    }

    return -1;
}

/* int32_t read_dentry_by_index(int32_t index, dentry_t* dentry); 
 * Inputs: index - index of dir-entry in boot block
 *			dentry - pointer to dir-entry that will be filled by 
 *					the dir-entry of the input file
 * Return Value: 0 for success, -1 for failure
 * Function: Retrieves the index-th entry in the boot-block's 
 *				dir-entry list
 */
int32_t read_dentry_by_index(uint32_t index, dentry_t* dentry) {
    if(!check_valid_file_type(root.dentries[index].file_type)) {
        return -1;
    }
    memcpy(dentry, &(root.dentries[index]), DENTRY_SIZE);
	return 0;
}


/* uint8_t check_valid_file_type(uint32_t file_type);
 * Inputs: file_type - a file type
 * Return Value: 1 for valid, 0 for invalid
 * Function: Returns 1 for a known file type, 0 for unknown
 */
uint8_t check_valid_file_type(uint32_t file_type) {
	switch(file_type) {
		case FILE_TYPE_RTC:
		case FILE_TYPE_DIR:
		case FILE_TYPE_REGULAR:
			return 1;
		default:
			break;
	}
	return 0;
}

