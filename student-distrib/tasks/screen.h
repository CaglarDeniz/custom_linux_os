/* screen.h - Functions headers relating to screens
 * vim:ts=4 noexpandtab
 */

#ifndef SCREEN_H
#define SCREEN_H

#include "../tty_structs.h"

#define MAX_SCREENS 3

void change_process_screen(int n);
void change_view_screen(int n);

extern int process_screen;
extern int view_screen;
extern screen_t screens[MAX_SCREENS];

#endif /* SCREEN_H */

