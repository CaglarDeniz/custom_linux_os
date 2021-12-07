/* rtc.h - Definitions for the Real-Time Clock
 * vim:ts=4 noexpandtab
 */


/* https://stanislavs.org/helppc/cmos_ram.html has useful info about registers*/

// Values to select registers
#define REG_A 0x0A
#define REG_B 0x0B
#define REG_C 0x0C

// RTC Mem IO ports
#define RTC_REG 0x70
#define RTC_DATA 0x71
