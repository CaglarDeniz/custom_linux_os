/* loader.h - Headers for loader
 * vim:ts=4 noexpandtab */

#ifndef LOADER_H
#define LOADER_H

#include "lib.h"
#include "filesystem/filesystem.h"

#define ELVEN_HEAD_SIZE 4
#define ELVEN_TAIL 0

#define PROGRAM_START 0x08048000

/* See loader.c for more detailed descriptions */
int check_header(dentry_t* dentry);
int is_executable_file(const uint8_t* filename);
int load_program(const uint8_t* filename);
void * get_start(void);

#endif // LOADER_H
