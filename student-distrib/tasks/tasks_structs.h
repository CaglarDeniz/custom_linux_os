/* tasks_structs.h - Defines structs used in implementing tasks
 * vim:ts=4 noexpandtab
 */

#ifndef TASKS_STRUCTS_H
#define TASKS_STRUCTS_H

#include "../syscalls/syscalls.h"

#define MAX_ACTIVE_TASKS 3

pcb_t *current_tasks[MAX_ACTIVE_TASKS]; 

#endif /* TASKS_STRUCTS_H */

