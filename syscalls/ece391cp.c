#include <stdint.h>

#include "ece391support.h"
#include "ece391syscall.h"

#define BUFSIZE 1024
#define MAX_LENGTH 34

int main ()
{
    int32_t cnt, i, j, fd_from, fd_to;
    uint8_t buf[6000];

    uint8_t file_args[BUFSIZE];
    uint8_t from[BUFSIZE], to[BUFSIZE];
  
    for(i = 0; i < BUFSIZE; ++i) {
      file_args[i] = 0;
      from[i] = 0;
      to[i] = 0;
    }

    if (0 != ece391_getargs (file_args, BUFSIZE)) {
      ece391_fdputs (1, (uint8_t*)"could not read argument\n");
      return 3;
    }

    for(i = 0; i < BUFSIZE && file_args[i] != ' ' && file_args[i] != '\0'; ++i) from[i] = file_args[i];
    from[i+1] = 0;

    for(; i < BUFSIZE && file_args[i] == ' ' && file_args[i] == '\0'; ++i);
    i += 1;

    for(j = 0; (i+j) < BUFSIZE && file_args[i+j] != '\0'; ++j) {
      to[j] = file_args[i+j];
    }
    to[j+1] = 0;

    if (-1 == (fd_from = ece391_open(from))) {
      ece391_fdputs (1, (uint8_t*)"file not found\n");
	    return 2;
    }
    fd_to = ece391_creat(to);

    while (0 != (cnt = ece391_read (fd_from, buf, 6000))) { // skecthcy :)
        if (-1 == cnt) {
	        ece391_fdputs (1, (uint8_t*)"file read failed\n");
	        return 3;
        }
        ece391_fdputs (fd_to, buf);
    }
    return 0;
}
