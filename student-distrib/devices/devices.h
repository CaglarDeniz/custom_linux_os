/* devices.h - Centralized header file for devices 
 *			that is accessed by other parts of the system
 * vim:ts=4 noexpandtab
 */

#include "../filesystem/filesystem.h"

#ifndef DEVICES_H
#define DEVICES_H

/* RTC constants */
#define RTC_MAX 0x06
#define OS_RTC_MAX 1024
#define OS_DEFAULT_RATE 32
#define LOW_BITS 0x0000FFFF
#define HIGH_BITS 0xFFFF0000
#define RTC_IRQ 0x08

/* Initialize the keyboard */
void keyboard_init(void);

/* Initialize the Real-Time Clock */
void rtc_init(void);

int32_t rtc_open(const uint8_t*);
int32_t rtc_close(int32_t fd);
int32_t rtc_read(int32_t fd, void* buf, int32_t nbytes);
int32_t rtc_write(int32_t fd, const void* buf, int32_t nbytes);
uint32_t rtc_wait(uint32_t duration_ms);
uint32_t rtc_check(uint32_t stop);

extern file_ops_t file_ops_rtc;

#endif
