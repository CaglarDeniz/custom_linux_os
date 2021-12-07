/* keyboard.c - Keyboard initialization and handler code
 * vim:ts=4 noexpandtab
 */

#include "../i8259.h"
#include "../lib.h"
#include "../tty.h"
#include "devices.h"
#include "keyboard.h"
#include "../tasks/tasks.h"
#include "../tasks/screen.h"

static uint8_t lshift_pressed = 0;
static uint8_t rshift_pressed = 0;
static uint8_t capslock_pressed = 0;
static uint8_t lctrl_pressed = 0;
static uint8_t rctrl_pressed = 0;
static uint8_t lalt_pressed = 0;
static uint8_t ralt_pressed = 0;

static void keyboard_handler();

void keyboard_init() {
  register_interrupt_handler(0x01, &keyboard_handler); // Register handler 1
}

int is_alpha(char c) {
	return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z');
}

static void keyboard_handler() {
  // operate on currently viewed screen
  change_process_screen(view_screen);
  // read scancode from keyboard
  uint8_t key_press = inb(KBD_PORT);
  //printf("[%x]",key_press);
  // determine if scancode indicates key press or key release
  uint8_t press = !(key_press & RELEASED);
  // For certain keys (such as arrows and RCTRL), the scan code is two bytes
  //     and starts with 0xE0. (there is a single scan code that that is
  //     three bytes and starts with 0xE1)
  if (key_press == KBD_PREFIX) {
    key_press = inb(KBD_PORT);
    press = !(key_press & RELEASED);
    key_press &= ~RELEASED;
    switch (key_press) {
      case KBD_LCTRL:
        rctrl_pressed = press;
        break;
      case KBD_LALT:
        ralt_pressed = press;
        break;
      default:
        if (0 && press) printf("[0x%x]",key_press);
    }
  } else {
    // mask out RELEASE bit
    key_press &= ~RELEASED;
    switch (key_press) {
      case KBD_RSHIFT: // shift pressed when key is pressed
        rshift_pressed = press;
        break;
      case KBD_LSHIFT: // shift pressed when key is pressed
        lshift_pressed = press;
        break;
      case KBD_CAPSLOCK: // shift toggled when key is pressed
        if (press) capslock_pressed = !capslock_pressed;
        break;
      case KBD_LCTRL: // ctrl pressed when key is pressed
        lctrl_pressed = press;
        break;
      case KBD_LALT: // ctrl pressed when key is pressed
        lalt_pressed = press;
        break;
      case KBD_UP:
        if (press) history_up();
        break;
      case KBD_DOWN:
        if (press) history_down();
        break;
      case KBD_BACKSPACE: // directly call backspace
        if (press) tty_backspace();//sendchar('\b');
        break;
      case KBD_F1:
        if (press && (lalt_pressed || ralt_pressed)) switch_view_screen(0);
        break;
      case KBD_F2:
        if (press && (lalt_pressed || ralt_pressed)) switch_view_screen(1);
        break;
      case KBD_F3:
        if (press && (lalt_pressed || ralt_pressed)) switch_view_screen(2);
        break;
      default:
        if (!press) break;
        if (lctrl_pressed || rctrl_pressed) {
          // for ctrl we send stuff
          // e.g.  ctrl+C sends '\x03'
          tty_sendchar(ctrl_chars[key_press]);
        } else if ((rshift_pressed || lshift_pressed) ^ 
        	(capslock_pressed && is_alpha(printable_lower[key_press]))) {
          tty_sendchar(printable_upper[key_press]);
        } else {
          tty_sendchar(printable_lower[key_press]);
        }
    }
  }
  // reset process screen
  change_process_screen(current_task);
}


