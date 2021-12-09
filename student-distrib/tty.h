/* tty.h - Defines function headers implemented by tty.c
 * vim:ts=4 noexpandtab
 */

#ifndef _TTY_H
#define _TTY_H

#include "lib.h"
#include "filesystem/filesystem.h"

#define MAX_TERMINAL_BUF_SIZE 128
#define MAX_TERMINAL_HIST_LEN 32

void scroll(void);
void set_cursor(int x, int y);
void set_cursor_default();
uint8_t tty_putc(uint8_t c);
uint8_t tty_putc_nocursor(uint8_t c);
int32_t tty_puts(int8_t* s);
void tty_backspace(void);
void tty_sendchar(uint8_t c);
int32_t tty_write(int32_t fd, const void* buf, int32_t nbytes);
int32_t tty_read(int32_t fd, void* buf, int32_t nbytes);
int32_t tty_open(const uint8_t* filename);
int32_t tty_close(int32_t fd);
void tty_clear_buf(void);
uint8_t is_printable(uint8_t c);
void save_data(void);
void load_data(void);
void tty_init(void);
void tab_complete(void);
void history_up(void);
void history_down(void);
void history_new(void);
uint8_t tty_set_attrib(uint8_t a);

extern file_ops_t file_ops_tty;

#endif /* _TTY_H */
