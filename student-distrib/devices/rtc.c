/* rtc.c - Initialization and handler code for Real-Time Clock
 * vim:ts=4 noexpandtab
 */

#include "../i8259.h"
#include "../lib.h"

#include "devices.h"
#include "rtc.h"

file_ops_t file_ops_rtc = {rtc_read, rtc_write, rtc_open, rtc_close};

static void rtc_handler();
static int32_t get_new_freq(const void* buf);

static uint32_t counter = 0;

typedef struct handler {
  void(*function)(uint32_t);
  uint32_t time;
  uint32_t arg;
} handler_t;

handler_t handlers[256];
uint32_t num_handlers = 0;


/*
 * rtc_init
 *    DESCRIPTION: Initializes the RTC at IRQ 8
 *    INPUTS: None
 *    OUTPUTS: None
 *    SIDE EFFECTS: Adds rtc_handler to IRQ 8 handler
 */
void rtc_init() {
  uint8_t prev, rate;

  outb(REG_B, RTC_REG);
  prev = inb(RTC_DATA); // Get current Reg B
  outb(REG_B, RTC_REG);
  outb(prev | 0x40, RTC_DATA); // Adjust clock settings (0x40 to bitmask)

  // Adjust the rate of interrupts to make grading easier :)
  rate = RTC_MAX; // Initialize to fastest allowable speed
  outb(REG_A, RTC_REG);
  prev = inb(REG_A);
  outb(REG_A, RTC_REG);
  outb((prev & 0xF0) | rate, RTC_DATA); // USE 0xF0 to bitmask

  register_interrupt_handler(RTC_IRQ, (void*) rtc_handler);
}

/* 
 * rtc_handler
 *    DESCRIPTION: Handler for RTC interrupts
 *    INPUTS: None
 *    OUTPUTS: None
 *    SIDE EFFECTS: Reads RTC reg C and calls test_interrupts
 */
static void rtc_handler() {
  ++counter;

  int i;
  for (i = 0; i < num_handlers; ++i) {
    if (handlers[i].time < counter) {
      handlers[i].function(handlers[i].arg);
      handlers[i--] = handlers[--num_handlers];
    }
  }

  // Need to read data or it won't unmask
  outb(REG_C, RTC_REG);
  inb(RTC_DATA);
}

/*
 * rtc_open
 *    DESCRIPTION: open function for RTC driver
 *    INPUTS: None
 *    OUTPUTS: None
 *    SIDE EFFECTS: Sets the default OS frequency
 */
int32_t rtc_open(const uint8_t* filename) {
  int32_t fd;
  fd = (int32_t) filename; // pls don't call CPS we'd never abuse our data
  fd_table[fd].flags |= OS_DEFAULT_RATE;
  fd_table[fd].file_pos = counter;
  return 0;
}

/*
 * rtc_close
 *    DESCRIPTION: close function for RTC driver
 *    INPUTS: None
 *    OUTPUTS: None
 *    SIDE EFFECTS: Removes handler
 */
int32_t rtc_close(int32_t fd) {
  return 0;
}

/*
 * rtc_read
 *    DESCRIPTION: read function for RTC driver
 *    INPUTS: None
 *    OUTPUTS: None
 *    SIDE EFFECTS: wait until interrupt happens
 */
int32_t rtc_read(int32_t fd, void* buf, int32_t nbytes) {
  int32_t curr_fd = fd;
  uint32_t c = fd_table[curr_fd].file_pos;
  fd_table[curr_fd].flags &= LOW_BITS;
  uint32_t f = fd_table[curr_fd].flags;
  while(1) {
   if(f < counter - c) break;
  }
  fd_table[curr_fd].file_pos += f;
  return 0;
}

/*
 * rtc_write
 *    DESCRIPTION: write function for RTC driver
 *    INPUTS: None
 *    OUTPUTS: None
 *    SIDE EFFECTS: changes RTC frequency
 */
int32_t rtc_write(int32_t fd, const void* buf, int32_t nbytes) {
  int32_t rate, i;

  rate = get_new_freq(buf);
  if(rate != (rate & (-1 * rate))) return -1; // Check power of 2
  else if(!(rate <= OS_RTC_MAX)) return -1; // Check not too big
  for(i = 0; rate > 1; ++i) rate = rate >> 1; // Find pwer of 2

  for(i = (16-RTC_MAX-i); i > 0; i--) rate *= 2; // 16 is the maximum exponent
  fd_table[fd].flags = rate;
  return 0;
}

/* 
 * get_new_freq
 *    DESCRIPTION: fetches new frequency from buffer
 *    INPUTS: None
 *    OUTPUTS: new frequency
 *    SIDE EFFECTS: None
 */
int32_t get_new_freq(const void* buf) {
  return (*((int32_t*) buf));
}

// TODO
uint32_t rtc_wait(uint32_t duration_ms) {
  return counter + (duration_ms*OS_RTC_MAX)/1000;
}

// TODO
uint32_t rtc_check(uint32_t stop) {
  return stop > counter;
}

// TODO
uint32_t rtc_register_handler(void(*function)(uint32_t), uint32_t arg, uint32_t wait) {
  int n = num_handlers++;
  int t = rtc_wait(wait);
  handlers[n].function = function;
  handlers[n].arg = arg;
  handlers[n].time = t;
  return t;
}
