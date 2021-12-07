/* screen.c - Functions to change screens
 * vim:ts=4 noexpandtab
 */

#include "../paging.h"
#include "../tty_structs.h"
#include "../tty.h"
#include "screen.h"

static uint8_t* video_mem = (uint8_t *)VIDEO;
static uint8_t* vidmap_mem = (uint8_t *)VIDMAP_MEM_ADDR;

#define VIDEO_MEMORY_SIZE (NUM_COLS*NUM_ROWS*2)

/* Current screen being viewed in terminal */
int view_screen = 0;
/* Current screen being operated on */
int process_screen = 0;

screen_t screens[MAX_SCREENS];

/* void change_process_screen(int n)
 * Inputs: n -- process_screen to change to
 * Return Value: none
 * Function: Updates terminal variables and page mappings
 */
void change_process_screen(int n) {
  if ((n < 0) || (n >= MAX_SCREENS)) return;
  save_data();
  if (n == view_screen) {
  	/* If the process screen is being viewed, map to video memory */
    map_video_to_video();
    map_vidmap_to_video();
  } else {
  	/* If the process screen is not being viewed, map to backup memory */
    map_video_to_backup(n);
    map_vidmap_to_backup(n);
  }
  process_screen = n;
  load_data();
}

/* void change_view_screen(int n)
 * Inputs: n -- view_screen to change to
 * Return Value: none
 * Function: Updates terminal variables and page mappings and copies backups
 */
void change_view_screen(int n) {
  if ((n < 0) || (n >= MAX_SCREENS)) return;
  if (n == view_screen) return;
  /* Use vidmap address as temporary pointer */
  enable_vidmap();
  /* Copy current Video memory into backup */
  map_video_to_video();
  map_vidmap_to_backup(view_screen);
  memcpy(vidmap_mem, video_mem, VIDEO_MEMORY_SIZE);
  /* Copy new backup into Video memeory */
  map_vidmap_to_backup(n);
  memcpy(video_mem, vidmap_mem, VIDEO_MEMORY_SIZE);
  view_screen = n;
  disable_vidmap();
  change_process_screen(process_screen);
  set_cursor(screens[view_screen].screen_x, screens[view_screen].screen_y);
}
