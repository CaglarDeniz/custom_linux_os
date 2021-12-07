#ifndef TTY_STRUCTS_H
#define TTY_STRUCTS_H

#include "lib.h"
#include "tty.h"

typedef struct {
  int screen_x;
  int screen_y;
  uint8_t width_buffer[MAX_TERMINAL_BUF_SIZE];
  uint8_t ready;
  uint8_t tab_ls;

  char input_history[MAX_TERMINAL_HIST_LEN][MAX_TERMINAL_BUF_SIZE];
  uint8_t size_history[MAX_TERMINAL_HIST_LEN];
  int history_start;
  int history_end;
  int history_curr;
} screen_t;

#endif

