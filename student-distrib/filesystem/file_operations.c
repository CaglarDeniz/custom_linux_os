/* file_operations.c - Implements file operations for regular files and 
 *						operation to read data from a file
 * vim:ts=4 noexpandtab
 */


#include "filesystem.h"

/* Definition of file operations table for regular files */
file_ops_t file_ops_regular = {file_read, file_write, file_open, file_close};

/* int32_t file_open(const uint8_t* filename);
 * Inputs: filename - name of file to be opened
 * Return Value: 0 for success, -1 for failure
 * Function: Does nothing 
 */
int32_t file_open(const uint8_t* filename) {
    return 0;
}

/* int32_t file_close(int32_t fd);
 * Inputs: fd - file descriptor of file to close
 * Return Value: 0 for success, -1 for failure
 * Function: Does nothing 
 */
int32_t file_close(int32_t fd) {
    return 0;
}

/* int32_t file_write(int32_t fd);
 * Inputs: fd - file descriptor of file to write to
 *			buf - buffer of string to write to the file
 *			nbytes - number of bytes in the string to write
 * Return Value: 0 for success, -1 for failure
 * Function: Does nothing, always fails since it is not supported
 */
int32_t file_write(int32_t fd, const void* buf, int32_t nbytes) {
    return -1; // TODO not yet supported
}

/* int32_t file_read(int32_t fd, void* buf, int32_t nbytes);
 * Inputs: fd - file descriptor of file to read from
 *			buf - buffer for string to be read from file
 *			nbytes - number of bytes to read
 * Return Value: number of bytes read
 * Function: Reads data from the file and moves the file_pos forward by the bytes read
 */
int32_t file_read(int32_t fd, void* buf, int32_t nbytes) {
	uint32_t bytes_read = read_data(fd_table[fd].inode_num, fd_table[fd].file_pos, buf, nbytes);
    fd_table[fd].file_pos += bytes_read;	/* Increment file position */
	return bytes_read;
}


/* int32_t read_data(uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length);
 * Inputs: inode - inode number of the file to have data read from
 *			offset - byte offset from the beginning of the file to begin reading from
 *			buf - buffer for string to be read from file
 *			nbytes - number of bytes to read 
 * Return Value: number of bytes read
 * Function: Loops through the data blocks of the file until 'length' bytes are read 
 *				or the end of file is reached
 */
int32_t read_data(uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length) {
    int32_t block_num;
    uint32_t num_bytes_read = 0;
    inode_t inode_block = inode_base[inode];
    void* current_block;
    const int32_t LAST_BLOCK_MAX_OFFSET = inode_block.length % BLOCK_SIZE;
    const int32_t NUM_BLOCKS = inode_block.length / BLOCK_SIZE + 
		(LAST_BLOCK_MAX_OFFSET == 0 ? 0 : 1); /* Don't forget last non-whole block */
	
    block_num = offset / BLOCK_SIZE;
    offset %= BLOCK_SIZE;

	/* Loop through data blocks until length bytes are read or end of file reached */
    while(1) {
        current_block = &(data_base[inode_block.data_blocks[block_num]]);
        
        /* We have reached the end of the file if we are past the last block 
        	or are past the last byte on the last block */
        if((block_num >= NUM_BLOCKS) || 
        		((block_num == NUM_BLOCKS - 1) && (offset >= LAST_BLOCK_MAX_OFFSET))) {
        	break;
        }
        
        /* If we are in the last block but don't have enough bytes left to fill the buffer */
        else if((block_num == NUM_BLOCKS - 1) && (offset + length >= LAST_BLOCK_MAX_OFFSET)) {
        	/* Note that offset < LAST_BLOCK_MAX_OFFSET because of the previous if statement */
        	memcpy(buf, current_block+offset, LAST_BLOCK_MAX_OFFSET-offset);
        	num_bytes_read += LAST_BLOCK_MAX_OFFSET - offset;
        	break;
        }
        
        /* Check if the current block contains enough bytes to fill the buffer */
        if(length < BLOCK_SIZE-offset) {
            memcpy(buf, current_block+offset, length);
            num_bytes_read += length;
            break;
        }

		/* If we cannot fill the buffer with just the current block, 
			read the rest of this block into the buffer and move on to the next block*/
        memcpy(buf, current_block+offset, BLOCK_SIZE-offset);
        length -= BLOCK_SIZE-offset;	        /* reduce bytes remaining to be read */
        buf += BLOCK_SIZE-offset;				/* move the location in the buffer to copy to */
        num_bytes_read += BLOCK_SIZE-offset;    /* track the number of bytes read */
        offset = 0;						        /* start at the beginning of the next block */
        block_num += 1;					        /* move to the next block */
    }

    return num_bytes_read;
}
