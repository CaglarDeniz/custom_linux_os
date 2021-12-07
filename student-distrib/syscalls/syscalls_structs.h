/* syscalls_structs.h - Defines structs used in implementing syscalls
 * vim:ts=4 noexpandtab
 */

#ifndef SYSCALLS_STRUCTS_H
#define SYSCALLS_STRUCTS_H

#include "../lib.h"
#include "../filesystem/filesystem.h"
#include "../filesystem/filesystem_structs.h"
#include "../tty.h" /* For MAX_TERMINAL_BUF_SIZE, screen_t */

#define PROCESS_STACK_SIZE EIGHT_KB

/* Struct for hardware context, which stores needed for returning from an irq */
typedef struct hw_context {
	struct hw_context* parent;
	uint32_t eax;
	uint32_t ebx;
	uint32_t ecx;
	uint32_t edx;
	uint32_t esi;
	uint32_t edi;
	uint32_t ebp;
	uint32_t irq_num;
	uint32_t err_code;
	uint32_t ret;
	uint32_t cs;
	uint32_t eflags;
	uint32_t esp; // not present if cs is not equal to USER_CS
	uint32_t ss; // not present if cs is not equal to USER_CS
} hw_context_t;


/* Struct for the Process Control Block, which stores state needed for transitioning between processes */
typedef struct pcb {
	fd_t process_fd_table[MAX_OPEN_FILES];	/* File descriptor table */
	uint32_t pid; 	/* Process id for current process */
	uint32_t pcb_num; /* 8kB slot number that this PCB occupies in memory */
	struct pcb *parent, *child; 	/* pointers to parent and child processes, NULL if doesn't exist */

	uint32_t user_physical_mem_block_num; 	/* Base address of User's 4 MB block of memory */
	
	uint8_t vidmap_enabled; /* Stores whether or not the current process is using vidmap */

	uint8_t command[MAX_TERMINAL_BUF_SIZE + 1]; /* Used for storing user command for use by get_args */

	hw_context_t *context; /* Context to return from interrupt/syscall */
} pcb_t;

#endif /* SYSCALLS_STRUCTS_H */
