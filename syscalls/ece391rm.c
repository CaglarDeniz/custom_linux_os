#include <stdint.h>

#include "ece391support.h"
#include "ece391syscall.h"

#define BUFSIZE 1024
#define MAX_LENGTH 34

int main ()
{
    int32_t cnt, i, j, fd_from, fd_to;
    uint8_t buf[6000];

    uint8_t file[BUFSIZE];
  
    for(i = 0; i < BUFSIZE; ++i) file[i] = 0;

    if (0 != ece391_getargs (file, BUFSIZE)) {
      ece391_fdputs (1, (uint8_t*)"could not read argument\n");
      return 3;
    }

    ece391_fdputs(1, (uint8_t*)"removing file: ");
    ece391_fdputs(1, file);
    ece391_fdputs(1, (uint8_t*)"\n");

    if (-1 == (ece391_unlink(file))) {
      ece391_fdputs (1, (uint8_t*)"file not found\n");
	    return 2;
    }

    return 0;
}
