/* tty.c - Implements terminal i/o functions
 * vim:ts=4 noexpandtab
 */

#include "i8259.h"
#include "lib.h"
#include "tty.h"
#include "filesystem/filesystem.h"
#include "tasks/screen.h"
#include "tasks/tasks.h"
#include "syscalls/syscalls.h"

#define VIDEO_CTRL_PORT 0x3D4
#define VIDEO_DATA_PORT 0x3D5
#define VIDEO_CURSOR_LOW 0x0F
#define VIDEO_CURSOR_HIGH 0x0E
#define VIDEO_CURSOR_MASK 0xFF

#define PROMPT_SIZE 7

#define CARET 0x40

static uint8_t caret = 0;
static uint8_t* video_mem = (uint8_t *)VIDEO;

static int screen_x;
static int screen_y;
static char* input_buffer;
static uint8_t* width_buffer;
static uint8_t buffer_size;
static volatile uint8_t ready;
static uint8_t tab_ls;
static int history_start;
static int history_end;
static int history_curr;

#define TAB_WIDTH 8

file_ops_t file_ops_tty = {tty_read, tty_write, tty_open, tty_close};

/* 
 * void scroll(void);
 * Inputs: void
 * Return Value: none
 * Function: Scrolls screen by 1 line (does not change cursor)
 */
void scroll(void) {
    int32_t i;
    // Copy last (NUM_ROWS-1) rows up
    for (i = 0; i < (NUM_ROWS-1) * NUM_COLS; i++) {
        *(video_mem + (i << 1)) = *(uint8_t *)(video_mem + ((i + NUM_COLS) << 1));
        *(video_mem + (i << 1) + 1) = *(uint8_t *)(video_mem + ((i + NUM_COLS) << 1) + 1);
    }
    // Clear last row
    for (; i < NUM_ROWS * NUM_COLS; i++) {
        *(video_mem + (i << 1)) = ' ';
        *(video_mem + (i << 1) + 1) = ATTRIB;
    }
}

/*
 * void set_cursor(int x, int y);
 * Inputs: int x = column of cursor
 *         int y = row of cursor
 * Return Value: none
 * Function: Sets cursor to position on screen given by (x,y)
 */
void set_cursor(int x, int y) {
    // Tell VGA we will be sending the low byte
    outb(VIDEO_CURSOR_LOW, VIDEO_CTRL_PORT);
    // Send lower byte
    outb((uint8_t) ((NUM_COLS * y + x) & VIDEO_CURSOR_MASK), VIDEO_DATA_PORT);
    // Tell VGA we will be sending the high byte
    outb(VIDEO_CURSOR_HIGH, VIDEO_CTRL_PORT);
    // Send high byte
    outb((uint8_t) (((NUM_COLS * y + x) >> 8) & VIDEO_CURSOR_MASK), VIDEO_DATA_PORT);
}

/*
 * void set_cursor_default();
 * Inputs: none
 * Return Value: none
 * Function: Sets cursor to position on screen given by (screen_x,screen_y)
 */
void set_cursor_default() {
    if (process_screen == view_screen) {
        set_cursor(screen_x, screen_y);
    }
}

/* 
 * static uint8_t tty_echo(uint8_t c);
 * Inputs: uint8_t c = character to print
 * Return Value: width of character printed (e.g. tab prints more than one character)
 * Function: Output a character to the console and sets cursor (also prints stuff like ^C)
 */
static uint8_t tty_echo(uint8_t c) {
    uint8_t r;
    caret = 1;
    r = tty_putc(c);
    caret = 0;
    return r;
}

/* 
 * static uint8_t tty_echo_nocursor(uint8_t c);
 * Inputs: uint8_t c = character to print
 * Return Value: width of character printed (e.g. tab prints more than one character)
 * Function: Output a character to the console and doesnt' set cursor (also prints stuff like ^C)
 */
static uint8_t tty_echo_nocursor(uint8_t c) {
    uint8_t r;
    caret = 1;
    r = tty_putc_nocursor(c);
    caret = 0;
    return r;
}

/* 
 * uint8_t tty_putc(uint8_t c);
 * Inputs: uint8_t c = character to print
 * Return Value: width of character printed (e.g. tab prints more than one character)
 * Function: Output a character to the console and sets cursor
 */
uint8_t tty_putc(uint8_t c) {
    uint8_t n = tty_putc_nocursor(c);
    set_cursor_default();
    return n;
}

/* 
 * uint8_t tty_putc_nocursor(uint8_t c);
 * Inputs: uint8_t c = character to print
 * Return Value: width of character printed (e.g. tab prints more than one character)
 * Function: Output a character to the console and does not set cursor
 *           (Does change logical cursor)
 */
uint8_t tty_putc_nocursor(uint8_t c) {
    uint8_t n = 1;
    if ((c == '\n') || (c == '\r')) {
        // '\n' and '\r' both move cursor down and left
        n = NUM_COLS-screen_x;
        screen_y++;
        screen_x = 0;
    } else if (c == '\t') {
        // '\t' prints up to TAB_WIDTH spaces and at least 1
        tty_putc_nocursor(' ');
        while ((screen_x % TAB_WIDTH) != 0) {
            tty_putc_nocursor(' ');
            n++;
        }
    } else {
        if (is_printable(c)) {
            // Video memory takes 2 bytes per character
            *(video_mem + ((NUM_COLS * screen_y + screen_x) << 1)) = c;
            *(video_mem + ((NUM_COLS * screen_y + screen_x) << 1) + 1) = ATTRIB;
            screen_x++;
            // Wrap around cursor
            if (screen_x == NUM_COLS) {
                screen_x = 0;
                screen_y += 1;
            }
        } else if (caret && is_printable(c ^ CARET)) {
            n = 2;
            tty_putc_nocursor('^');
            tty_putc_nocursor(c ^ CARET);
        } else if (!caret && c) {
            *(video_mem + ((NUM_COLS * screen_y + screen_x) << 1)) = c;
            *(video_mem + ((NUM_COLS * screen_y + screen_x) << 1) + 1) = ATTRIB;
            screen_x++;
            // Wrap around cursor
            if (screen_x == NUM_COLS) {
                screen_x = 0;
                screen_y += 1;
            }
        } else {
            n = 0;
        }
    }
    // If cursor would be off the bottom of the screen, scroll down
    if (screen_y == NUM_ROWS) {
        scroll();
        screen_y--;
    }
    return n;
}

/* 
 * int32_t tty_puts(int8_t* s);
 * Inputs: int_8* s = pointer to a string of characters
 * Return Value: Number of bytes written
 * Function: Output a string to the console and sets cursor
 */
int32_t tty_puts(int8_t* s) {
    register int32_t index = 0; // lib.c did it
    while (s[index] != '\0') {
        tty_putc_nocursor(s[index]); // avoid moving cursor unnecessarily
        index++;
    }
    set_cursor_default();
    return index;
}

/*
 * static int check_shell(void);
 * INPUTS: none
 * RETURN VALUE: 1 if probably in shell, otherwise 0
 * Checks if current process is shell (by looking for 391OS> prompt)
 */
static int check_shell(void) {
    int x = screen_x - PROMPT_SIZE;
    int y = screen_y;
    int i;
    for (i = 0; i < buffer_size; i++) {
        x -= width_buffer[i];
        if (x < 0) {
            x += NUM_COLS;
            y -= 1;
        }
    }
    if (y < 0) return 0;
    for (i = 0; i < PROMPT_SIZE; i++) {
        if (*(video_mem + ((NUM_COLS * y + x + i) << 1)) != "391OS> "[i]) return 0;
    }
    return 1;
}

/*
 * void tty_cls(void);
 * Inputs: none
 * Return Value: none
 * Function: Clear screen and set cursor to top left and echo current buffer
 */
void tty_cls(void) {
    int32_t i;
    int in_shell = check_shell(); // lol
    for (i = 0; i < NUM_ROWS * NUM_COLS; i++) {
        *(video_mem + (i << 1)) = ' ';
        *(video_mem + (i << 1) + 1) = ATTRIB;
    }
    screen_x = 0;
    screen_y = 0;
    if (in_shell) tty_puts("391OS> ");
    for (i = 0; i < buffer_size; i++) {
        width_buffer[i] = tty_echo_nocursor(input_buffer[i]);
    }
    set_cursor_default();
}

/*
 * void tty_backspace(void);
 * Inputs: none
 * Return Value: none
 * Function: If screen_x > 0, executes a backspace
 */
void tty_backspace(void) {
    int i;
    // this gets messed up by printing stuff while typing
    if (buffer_size > 0) {
        if ((screen_x == 0) && (screen_y == 0)) return; // nowhere to go
        buffer_size--;
        for (i = 0; i < width_buffer[buffer_size]; i++) {
            screen_x--;
            if (screen_x < 0) {
                screen_x += NUM_COLS;
                screen_y -= 1;
            }
            // clear position
            *(video_mem + ((NUM_COLS * screen_y + screen_x) << 1)) = ' ';
            *(video_mem + ((NUM_COLS * screen_y + screen_x) << 1) + 1) = ATTRIB;
        }
        // Set new cursor position
        set_cursor_default();
    }
    tab_ls = 0;
}

/*
 * void tty_sendchar(uint8_t c);
 * Inputs: uint8_t c -- char to send to terminal driver
 * Return Value: none
 * Decides what to do with character sent by keyboard driver (or otherwise)
 */
void tty_sendchar(uint8_t c) {
    // This is called primarily by the keyboard driver
    switch (c) {
      case '\x0c': // ctrl+L gets sent as '\x0c'
        tty_cls();
        tab_ls = 0;
        break;
      case '\t':
        tab_complete();
        break;
      // case '\b': // backspace
      //   tty_backspace();
      //   break;
      case 255: // non printable
        // tab_ls = 0;
        break;
      default:
        if (buffer_size < MAX_TERMINAL_BUF_SIZE-1) { // can write (leave room for enter)
            input_buffer[buffer_size] = c;
            width_buffer[buffer_size] = tty_echo(c);
            buffer_size++;
            if (c == '\n') ready = 1;
        } else if (buffer_size == MAX_TERMINAL_BUF_SIZE-1) {
            if (c == '\n') { // only enter can be entered now
                tty_echo(c);
                input_buffer[buffer_size] = c;
                buffer_size++;
                ready = 1;
            }
        }
      tab_ls = 0;
    }
}

/*
 * int32_t tty_write(int32_t fd, const void* buf, int32_t nbytes);
 * Inputs: const void* buf -- buffer of chars to print (including null bytes)
 *         int32_t nbytes  -- Number of bytes to write
 * Return Value: number of bytes written
 * Prints a string to the screen
 */
int32_t tty_write(int32_t fd, const void* buf, int32_t nbytes) {
    if (fd == STDIN) return -1;
    if (buf == 0) return -1; // can't read from null
    int32_t i;
    const uint8_t* buf_c = buf;
    for (i = 0; i < nbytes; i++) {
        uint8_t c = buf_c[i];
        // how do we know that we're supposed to be printing this?
        // whatever
        tty_putc_nocursor(c);
    }
    set_cursor_default();
    return i;
}

/*
 * static void shift_buffer(uint8_t i);
 * Input: uint8_t i -- part of buffer to clear
 * Return Value: none
 * Clears i bytes from buffer, also clears ready if no enter in buffer
 */
static void shift_buffer(uint8_t i) {
    uint8_t j = 0;
    ready = 0;
    for (; i < buffer_size; i++) {
        input_buffer[j] = input_buffer[i];
        if (input_buffer[j] == '\n') { // buffer has '\n'
            ready = 1;
        }
        j++;
    }

    buffer_size = j;
    
    
}

/*
 * int32_t tty_read(int32_t fd, void* buf, int32_t nbytes);
 * Inputs: void* buf -- buffer to write bytes to
 *         int32_t nbytes -- number of bytes to read
 * Return Value: number of bytes read
 * Reads either up to a newline or 128 or nbytes bytes from the keyboard,
 * whichever comes first
 * will wait for newline (not if there is already a newline in buffer)
 */
int32_t tty_read(int32_t fd, void* buf, int32_t nbytes) {
    if (fd == STDOUT) return -1;
    if (buf == 0) return -1; // can't write to null
    int32_t i;
    uint8_t* buf_c = buf;
    for (i = 0; i < buffer_size; i++) {
        width_buffer[i] = tty_echo_nocursor(input_buffer[i]);
        if (input_buffer[i] == '\n') break;
    }
    set_cursor_default();
    while (!ready); // wait for newline
    for (i = 0; (i < nbytes) && (i < buffer_size); i++) {
        buf_c[i] = input_buffer[i];
        if (input_buffer[i] == '\n') { // we're done
            i++; // wrote '\n' too
            break;
        }
    }
    history_new();
    shift_buffer(i); // clear buffer
    //tty_clear_buf(); // clear whole buffer
    return i;
}

/*
 * int32_t tty_open(const uint8_t* filename);
 * Inputs: none
 * Return Value: 0
 * Initializes terminal (does nothing)
 */
int32_t tty_open(const uint8_t* filename) {
    return 0;
}

/*
 * int32_t tty_close(int32_t fd);
 * Inputs: none
 * Return Value: -1
 * Clears terminal stuff (does nothing)
 */
int32_t tty_close(int32_t fd) {
    return -1; // you can't close the terminal
}

/*
 * void tty_clear_buf(void);
 * Inputs: none
 * Return Value: none
 * Clears the input buffer
 */
void tty_clear_buf(void) {
    buffer_size = 0;
    ready = 0;
}

/*
 * uint8_t is_printable(uint8_t c);
 * Inputs: uint8_t c -- char to check
 * Return Value: 1 if c is printable, otherwise 0
 * Checks if a character is printable
 */
uint8_t is_printable(uint8_t c) {
    if (c == '\n') return 1;
    if (c == '\r') return 1;
    if (c == '\t') return 1;
    if ((c >= ' ') && (c <= '~')) return 1;
    return 0;
}

/*
 * void save_data(void);
 * INPUTS: none
 * RETURN VALUE: none
 * Saves current data to screens array
 */
void save_data(void) {
    screens[process_screen].screen_x = screen_x;
    screens[process_screen].screen_y = screen_y;
    screens[process_screen].ready = ready;
    screens[process_screen].tab_ls = tab_ls;
    screens[process_screen].history_start = history_start;
    screens[process_screen].history_end = history_end;
    screens[process_screen].history_curr = history_curr;
    screens[process_screen].size_history[history_curr] = buffer_size;
}

/*
 * void load_data(void);
 * INPUTS: none
 * RETURN VALUE: none
 * Loads current data from screens array
 */
void load_data(void) {
    history_start = screens[process_screen].history_start;
    history_end = screens[process_screen].history_end;
    history_curr = screens[process_screen].history_curr;
    screen_x = screens[process_screen].screen_x;
    screen_y = screens[process_screen].screen_y;
    input_buffer = screens[process_screen].input_history[history_curr];
    width_buffer = screens[process_screen].width_buffer;
    buffer_size = screens[process_screen].size_history[history_curr];
    ready = screens[process_screen].ready;
    tab_ls = screens[process_screen].tab_ls;
}

/*
 * void tty_init(void);
 * INPUTS: none
 * RETURN VALUE: none
 * Initializes tty
 */
void tty_init(void) {
    // Makes static variables point to screens array
    load_data();
}

/*
 * static int get_last(void);
 * INPUTS: none
 * RETURN VALUE: index of first character of last word
 * Determines the last word in the input
 */
static int get_last(void) {
    int i;
    for (i = buffer_size-1; i >= 0; i--) {
        if ((input_buffer[i] == ' ') || (input_buffer[i] == '\n') || (input_buffer[i] == '\t')) {
            return i+1;
        }
    }
    return 0;
}

/*
 * static void tty_ls(char* filename, int n);
 * INPUTS: char* filename -- Pointer to currently typed part of filename
 *         int n          -- Length of currently typed part of filename
 * RETURN VALUE: none
 * Prints all possible matches for tab completion
 */
static void tty_ls(char* filename, int n) {
    int fd = open((uint8_t*)".");
    char test[MAX_FILENAME_LENGTH+1] = {0};
    int m = 0;
    int i, c, j;
    int in_shell = check_shell(); // lol
    while (read(fd, test, MAX_FILENAME_LENGTH) > 0) {
        if (strncmp(test, filename, n) == 0) {
            i = strlen(test);
            if (i > m) {
                m = i;
            }
        }
    }
    close(fd);
    m += 2; // at least 2 spaces between columns
    for (c = 2; m*c+m < NUM_COLS; c++);
    // c is number of columns of filenames (at least 2)
    fd = open((uint8_t*)".");
    i = 0;
    while (read(fd, test, MAX_FILENAME_LENGTH) > 0) {
        if (strncmp(test, filename, n)) continue;
        if (i == 0) tty_putc_nocursor('\n');
        for (j = 0; j < m; j++) {
            if (test[j]) {
                tty_putc_nocursor(test[j]);
            } else {
                break;
            }
        }
        for (; j < m; j++) tty_putc_nocursor(' ');
        i += 1;
        if (i == c) i = 0;
    }
    tty_putc_nocursor('\n');
    // rewrite input ...
    if (in_shell) tty_puts("391OS> ");
    for (i = 0; i < buffer_size; i++) {
        width_buffer[i] = tty_echo_nocursor(input_buffer[i]);
    }
    set_cursor_default();
}

/*
 * void tab_complete(void);
 * INPUTS: none
 * OUTPUTS: none
 * Attempts to tab complete word
 */
void tab_complete(void) {
    int n = get_last();
    int i, m, single;
    char filename[MAX_FILENAME_LENGTH] = {0};
    char test[MAX_FILENAME_LENGTH+1] = {0};
    if (buffer_size - n > MAX_FILENAME_LENGTH) {
        return; // no completions possible
    }
    // copy filename
    for (i = n; i < buffer_size; i++) {
        filename[i-n] = input_buffer[i];
    }
    n = buffer_size - n;
    int fd = open((uint8_t*)".");
    single = 0;
    while (read(fd, test, MAX_FILENAME_LENGTH) > 0) {
        if (strncmp(test, filename, n) == 0) {
            for (m = n; (m < MAX_FILENAME_LENGTH) && (test[m]); m++) {
                filename[m] = test[m];
            }
            single = 1;
            break;
        }
    }
    if (single == 0) {
        // didn't find any match
        tab_ls = 0;
        close(fd);
        return;
    }
    while (read(fd, test, MAX_FILENAME_LENGTH) > 0) {
        if (strncmp(test, filename, n) == 0) {
            for (i = n; i < m; i++) {
                if (test[i] != filename[i]) {
                    m = i;
                    if (m == n) {
                        close(fd);
                        // multiple matches + nothing to add
                        if (tab_ls) tty_ls(filename, n);
                        tab_ls = 1;
                        // if user presses tab again, ls will happen (basically)
                        return;
                    }
                    single = 0;
                    break;
                }
            }
        }
    }
    // add anything extra
    for (i = n; i < m; i++) {
        tty_sendchar(filename[i]);
    }
    if (single) {
        // tty_sendchar(' '); // until after checkpoint 5
    }
    tab_ls = 0;
    close(fd);
}

/*
 * void rewrite(void);
 * INPUTS: none
 * RETURN VALUE: none
 * rewrites current input buffer (also fills width_buffer)
 */
void rewrite(void) {
    int i;
    for (i = 0; i < buffer_size; i++) {
        width_buffer[i] = tty_echo_nocursor(input_buffer[i]);
    }
    set_cursor_default();
}

/*
 * void history_up(void);
 * INPUTS: none
 * OUTPUTS: none
 * Scrolls up in history
 */
void history_up(void) {
    tab_ls = 0;
    if (history_curr == history_start) return;
    // save buffer size
    screens[process_screen].size_history[history_curr] = buffer_size;
    // clear current line
    while (buffer_size) tty_backspace();
    // load new line
    history_curr -= 1;
    if (history_curr < 0) history_curr += MAX_TERMINAL_HIST_LEN;
    input_buffer = screens[process_screen].input_history[history_curr];
    buffer_size = screens[process_screen].size_history[history_curr];
    // write new line
    rewrite();
}

/*
 * void history_down(void);
 * INPUTS: none
 * OUTPUTS: none
 * Scrolls down in history
 */
void history_down(void) {
    tab_ls = 0;
    if (history_curr == history_end) return;
    // save buffer size
    screens[process_screen].size_history[history_curr] = buffer_size;
    // clear current line
    while (buffer_size) tty_backspace();
    // load new line
    history_curr += 1;
    if (history_curr >= MAX_TERMINAL_HIST_LEN) history_curr -= MAX_TERMINAL_HIST_LEN;
    input_buffer = screens[process_screen].input_history[history_curr];
    buffer_size = screens[process_screen].size_history[history_curr];
    // write new line
    rewrite();
}

/*
 * void history_new(void);
 * INPUTS: none
 * OUTPUTS: none
 * Creates new history line
 */
void history_new(void) {
    // save buffer size (minus newline)
    screens[process_screen].size_history[history_curr] = buffer_size-1;
    // set new history_start/end
    if (history_curr + 1 != history_end) {
        // copy line
        if (history_end != history_curr)
            strncpy(screens[process_screen].input_history[history_end], input_buffer, MAX_TERMINAL_BUF_SIZE);
        screens[process_screen].size_history[history_end] = buffer_size-1;
        history_end += 1;
        if (history_end == MAX_TERMINAL_HIST_LEN) history_end = 0;
        if (history_end == history_start) history_start += 1;
    }
    // copy line
    if (history_end != history_curr)
        strncpy(screens[process_screen].input_history[history_end], input_buffer, MAX_TERMINAL_BUF_SIZE);
    history_curr = history_end;
    input_buffer = screens[process_screen].input_history[history_curr];
    buffer_size = screens[process_screen].size_history[history_curr];
}
