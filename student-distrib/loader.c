/* loader.c - Implements the program loader and adjacent functions
 * vim:ts=4 noexpandtab */

#include "lib.h"
#include "paging.h"
#include "filesystem/filesystem.h"
#include "loader.h"

#define BUF_SIZE 128
static uint8_t ELVEN_HEAD[ELVEN_HEAD_SIZE] = {'\x7f','E','L','F'};

/*
 * static int check_header(dentry_t* dentry);
 * Inputs: dentry_t* dentry -- dentry of program to load
 * Outputs: 1 if valid header, 0 otherwise
 * Checks header of file
 */
int check_header(dentry_t* dentry) {
  int i;
  uint8_t buf[ELVEN_HEAD_SIZE];
  if (dentry->file_type != FILE_TYPE_REGULAR) return 0;
  if (read_data(dentry->inode_num, 0, buf, ELVEN_HEAD_SIZE) < ELVEN_HEAD_SIZE)
    return 0;
  for (i = 0; i < ELVEN_HEAD_SIZE; i++) {
    if (buf[i] != ELVEN_HEAD[i]) return 0;
  }
  return 1;
}

/*
 * int is_executable_file(const uint8_t* filename);
 * Inputs: filename - filename of program to check 
 * Outputs: 1 on executable, 0 for not executable or error
 * Checks if a file is executable
 */
int is_executable_file(const uint8_t* filename) {
  dentry_t dentry;
  if (read_dentry_by_name(filename, &dentry)) return 0; /* Error */
  if (dentry.file_type != FILE_TYPE_REGULAR) return 0; /* Not exec */
  if (!check_header(&dentry)) return 0; /* Not exec */
  return 1;
}

/*
 * int load_program(const uint8_t* filename);
 * Inputs: const uint8_t* filename = filename of program to load
 * Outputs: 0 on success, negative on error
 * Loads a program by filename into memory at 0x08048000.
 * First checks that the file is a file and the magic number is present
 * The page must be setup before calling this
 */
int load_program(const uint8_t* filename) {
  dentry_t dentry;
  uint8_t buf[BUF_SIZE];
  uint32_t i, j, k;
  if(!is_executable_file(filename)) {
  	return -1;
  }
  read_dentry_by_name(filename, &dentry);
  check_header(&dentry);
  i = 0; 
  while ((k = read_data(dentry.inode_num, i, buf, BUF_SIZE)) > 0) {
    for (j = 0; j < k; j++) {
      *(uint8_t*)(PROGRAM_START+i+j) = buf[j];
    }
    i += k;
  }
  return 0;
}

/*
 * void * get_start(void);
 * Inputs: None
 * Output: program start
 * Gets the starting location of a program loaded in memory
 */
void * get_start(void) {
  return *(void**)(PROGRAM_START+24);
}
