/* lib.h - Defines for useful library functions
 * vim:ts=4 noexpandtab
 */

#ifndef _LIB_H
#define _LIB_H

#include "types.h"

#define VIDEO       0xB8000 /* Location of video memory */

/* Dimensions of Terminal */
#define NUM_COLS    80
#define NUM_ROWS    25
#define ATTRIB      0x5

#define FOUR_MB (1 << 22)
#define FOUR_KB (1 << 12)
#define EIGHT_KB (FOUR_KB << 1)
#define EIGHT_MB (FOUR_MB << 1)

int32_t printf(int8_t *format, ...);
void putc(uint8_t c);
int32_t puts(int8_t *s);
int8_t *itoa(uint32_t value, int8_t* buf, int32_t radix);
int atoi(int8_t* s, int* num);
int8_t *strrev(int8_t* s);
uint32_t strlen(const int8_t* s);
void clear(void);

void* memset(void* s, int32_t c, uint32_t n);
void* memset_word(void* s, int32_t c, uint32_t n);
void* memset_dword(void* s, int32_t c, uint32_t n);
void* memcpy(void* dest, const void* src, uint32_t n);
void* memmove(void* dest, const void* src, uint32_t n);
int32_t strncmp(const int8_t* s1, const int8_t* s2, uint32_t n);
int8_t* strcpy(int8_t* dest, const int8_t*src);
int8_t* strncpy(int8_t* dest, const int8_t*src, uint32_t n);
uint32_t strlcpy(int8_t* dest, const int8_t* src, uint32_t n);

/* Userspace address-check functions */
int32_t bad_userspace_addr(const void* addr, int32_t len);
int32_t safe_strncpy(int8_t* dest, const int8_t* src, int32_t n);

/* TODO convenience? */
void die(int8_t* s);
uint32_t min(uint32_t a, uint32_t b);

void test_interrupts(void);

/* Port read functions */
/* Inb reads a byte and returns its value as a zero-extended 32-bit
 * unsigned int */
static inline uint32_t inb(int port) {
    uint32_t val;
    asm volatile ("             \n\
            xorl %0, %0         \n\
            inb  (%w1), %b0     \n\
            "
            : "=a"(val)
            : "d"(port)
            : "memory"
    );
    return val;
}

/* Reads two bytes from two consecutive ports, starting at "port",
 * concatenates them little-endian style, and returns them zero-extended
 * */
static inline uint32_t inw(int port) {
    uint32_t val;
    asm volatile ("             \n\
            xorl %0, %0         \n\
            inw  (%w1), %w0     \n\
            "
            : "=a"(val)
            : "d"(port)
            : "memory"
    );
    return val;
}

/* Reads four bytes from four consecutive ports, starting at "port",
 * concatenates them little-endian style, and returns them */
static inline uint32_t inl(int port) {
    uint32_t val;
    asm volatile ("inl (%w1), %0"
            : "=a"(val)
            : "d"(port)
            : "memory"
    );
    return val;
}

/* Writes a byte to a port */
#define outb(data, port)                \
do {                                    \
    asm volatile ("outb %b1, (%w0)"     \
            :                           \
            : "d"(port), "a"(data)      \
            : "memory", "cc"            \
    );                                  \
} while (0)

/* Writes two bytes to two consecutive ports */
#define outw(data, port)                \
do {                                    \
    asm volatile ("outw %w1, (%w0)"     \
            :                           \
            : "d"(port), "a"(data)      \
            : "memory", "cc"            \
    );                                  \
} while (0)

/* Writes four bytes to four consecutive ports */
#define outl(data, port)                \
do {                                    \
    asm volatile ("outl %1, (%w0)"     \
            :                           \
            : "d"(port), "a"(data)      \
            : "memory", "cc"            \
    );                                  \
} while (0)

/* Clear interrupt flag - disables interrupts on this processor */
#define cli()                           \
do {                                    \
    asm volatile ("cli"                 \
            :                           \
            :                           \
            : "memory", "cc"            \
    );                                  \
} while (0)

/* Save flags and then clear interrupt flag
 * Saves the EFLAGS register into the variable "flags", and then
 * disables interrupts on this processor */
#define cli_and_save(flags)             \
do {                                    \
    asm volatile ("                   \n\
            pushfl                    \n\
            popl %0                   \n\
            cli                       \n\
            "                           \
            : "=r"(flags)               \
            :                           \
            : "memory", "cc"            \
    );                                  \
} while (0)

/* Set interrupt flag - enable interrupts on this processor */
#define sti()                           \
do {                                    \
    asm volatile ("sti"                 \
            :                           \
            :                           \
            : "memory", "cc"            \
    );                                  \
} while (0)

/* Restore flags
 * Puts the value in "flags" into the EFLAGS register.  Most often used
 * after a cli_and_save_flags(flags) */
#define restore_flags(flags)            \
do {                                    \
    asm volatile ("                   \n\
            pushl %0                  \n\
            popfl                     \n\
            "                           \
            :                           \
            : "r"(flags)                \
            : "memory", "cc"            \
    );                                  \
} while (0)

#define save_esp(regs)					\
do {                                    \
    asm volatile ("                   \n\
            pushl %%esp				  \n\
            popl %0                   \n\
            "                           \
            : "=r"(regs)               \
            :                           \
            : "memory", "cc"            \
    );                                  \
} while (0)

#define restore_esp(regs)				\
do {                                    \
    asm volatile ("                   \n\
            pushl %0				  \n\
            popl %%esp                \n\
            "                           \
			:							\
            :"r"(regs)					\
            : "esp", "memory", "cc"            \
    );                                  \
} while (0)

#define save_ebp(regs)					\
do {                                    \
    asm volatile ("                   \n\
            pushl %%ebp				  \n\
            popl %0                   \n\
            "                           \
            : "=r"(regs)               \
            :                           \
            : "memory", "cc"            \
    );                                  \
} while (0)

#define restore_ebp(regs)				\
do {                                    \
    asm volatile ("                   \n\
            pushl %0				  \n\
            popl %%ebp                \n\
            "                           \
			:							\
            :"r"(regs)					\
            : "ebp", "memory", "cc"            \
    );                                  \
} while (0)

#endif /* _LIB_H */
