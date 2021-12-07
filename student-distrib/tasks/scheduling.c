/* scheduling.c - Implements round-robing scheduling
 * vim:ts=4 noexpandtab
 */

#include "scheduling.h"
#include "tasks.h"

static uint32_t rr_counter = 0 ;
static uint8_t curr_process_index = 0 ;

/* static round robin queue */
static pcb_t* rr_queue[MAX_QUEUE_SIZE] = {} ; 

/* void round_robin()
 * Inputs: 
 * Return Value: none
 * Function: The PIT counter function for scheduling 
 */

void round_robin() {

    /* if current processes time slot has not ended*/
    if (rr_counter != RR_PERIOD) {
        // printf("%d \n ",rr_counter);
        rr_counter+=1 ; // increment counter
        return ;
    }

    else { /* do context switch */ 

        curr_process_index = (curr_process_index+1) % MAX_QUEUE_SIZE;

        switch_process(curr_process_index);

        rr_counter = 0 ; // reset the counter
        return ; 
    }
}

/* void rr_push(pcb_t* new_process)
 * Inputs: pcb_t* new_process
 * Return Value: none
 * Function: pushes a new processes pcb to the next free spot on the scheduling queue
 */

void rr_push(pcb_t* new_process) { 

    int free_index  = next_free() ; 
    /* push the new_process to the rr_queue*/ 
    rr_queue[free_index] = new_process ; 
}


/* pcb_t* rr_pop(uint8_t index)
 * Inputs: uint8_t index
 * Return Value: pcb_t* popped_pcb
 * Function: Pops the process at the given index from the scheduling queue
 */

pcb_t*  rr_pop(uint8_t index)  { 

    int i;

    pcb_t *popped_pcb = rr_queue[index] ;  

    /* if the rr queue is empty at the index, return SCHEDULING ERROR */ 
    if (!rr_queue[index]) return SCHEDULING_ERROR ; 

    /* else take the process off the queue, move the queue up */ 

        // takes the process off the rr queue
        rr_queue[index] = NULL ; 

        // while the in the queue bound and there are processes left
        for ( i = index+1 ; rr_queue[i] != NULL && i < MAX_QUEUE_SIZE ; i++ ) { 

            /* shift process at i to the left by 1 */
            rr_queue[i-1] = rr_queue[i] ; 
        }

    return popped_pcb ;
}

/* uint8_t next_free()
 * Inputs: None
 * Return Value: uint8_t free_index
 * Function: Returns the index for the next free spot in the scheduling queue
 */

uint8_t next_free() { 

    int i , free_index ; 

     // while in bounds of the array 
    for ( i = 0 ; i < MAX_QUEUE_SIZE ; i++) 

        // if pcb pointer empty, record the current index as free_index
        if (!rr_queue[i]) { 
            free_index = i ; 
            break ; 
        }

    return free_index; 
}

