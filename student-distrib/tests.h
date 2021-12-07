#ifndef TESTS_H
#define TESTS_H

#define TEST_NUM 0 // 0 = Boot, 1 = IDT hardware int, 2 = IDT syscall, 3 = RTC

#define BUF_SIZE 200 /* Buffer size used for tty testing */

// test launcher
void launch_tests();

#endif /* TESTS_H */
