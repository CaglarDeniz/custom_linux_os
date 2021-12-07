/* scheduling.h - Interface for scheduling used by the rest of the kernel
 * vim:ts=4 noexpandtab
 */

#ifndef SCHEDULING_H
#define SCHEDULING_H

#include "../syscalls/syscalls.h"

#define MAX_QUEUE_SIZE 3 
#define SCHEDULING_ERROR 0x0
#define RR_PERIOD 10 /* scheduling slice in terms of RTC ticks*/

/* function constantly calls execute in the round robin scheduling scheme */
void round_robin() ;

/* pushes a new process to the end of the  round robin queue */ 
void rr_push(pcb_t* new_proccess) ; 

/* pops a process from the round robin queue at the given index*/ 
pcb_t* rr_pop(uint8_t index) ; 

/* return index for next free index  */
uint8_t next_free() ; 

#endif /* SCHEDULING_H */
