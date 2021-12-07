/* syscalls.h - Defines function headers for all syscalls
 * vim:ts=4 noexpandtab
 */

#ifndef SYSCALLS_H
#define SYSCALLS_H

#include "../types.h"
#include "../filesystem/filesystem.h"
#include "syscalls_structs.h" /* Included for pcb_t */

#define MAX_PROCESSES 6
#define SYSCALL_ERROR -1

int32_t halt(uint8_t status);
int32_t execute(const uint8_t* command);
int32_t read(int32_t fd, void* buf, int32_t nbytes);
int32_t write(int32_t fd, const void* buf, int32_t nbytes);
int32_t open(const uint8_t* filename);
int32_t close(int32_t fd);
int32_t getargs(uint8_t* buf, int32_t nbytes);
int32_t vidmap(uint8_t** screen_start);
int32_t set_handler(int32_t signum, void* handler_address);
int32_t sigreturn(void);
void setup_fdtable(fd_t* fd_table);
int32_t syscall_shim(int32_t b, int32_t c, int32_t d, int32_t a);

/* See process.c for more information */
extern pcb_t *current_pcb; 
uint32_t push_pcb(void);
uint32_t pop_pcb(void);
uint32_t is_in_user_mem(uint32_t addr);
void update_tss(void);

#endif /* SYSCALLS_H */
