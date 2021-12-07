/* tasks.h - Defines function headers and global variables for all tasks
 * vim:ts=4 noexpandtab
 */

#ifndef TASKS_H
#define TASKS_H

#include "../types.h"
#include "tasks_structs.h" 

/* Switches active task that displays to terminal */
int32_t switch_view_screen(int task_id);

/* Switches currently executing process to the target process */
int32_t switch_process(int task_id);

extern pcb_t *current_tasks[MAX_ACTIVE_TASKS]; 
int current_task;
#define USER_MEM_END (FOUR_MB * (USER_MEM_PAGE_INDEX + 1)) // TODO remove


#endif /* TASKS_H */
