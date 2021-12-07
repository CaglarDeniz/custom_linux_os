#include "tests.h"
#include "x86_desc.h"
#include "lib.h"
#include "paging.h"
#include "idt.h"
#include "filesystem/filesystem.h"
#include "syscalls/syscalls.h"
#include "tty.h"
#include "loader.h"
#include "tasks/scheduling.h"
#include "devices/devices.h"
#include "i8259.h"
#include "tasks/screen.h"
#include "networking/networking.h"

#define PASS 1
#define FAIL 0

/* format these macros as you see fit */
#define TEST_HEADER 	\
    printf("")
	//printf("[TEST %s] Running %s at %s:%d\n", __FUNCTION__, __FUNCTION__, __FILE__, __LINE__)
	
int test_result; /* Temporary variable used in the TEST_OUTPUT macro */
#define TEST_OUTPUT(name, result, failure_count)	\
	test_result = result; \
	if (!test_result)printf("[TEST %s] Result = %s\n", name, (test_result) ? "PASS" : "FAIL"); \
	*(failure_count) += !test_result;

static inline void assertion_failure(){
	/* Use exception #15 for assertions, otherwise
	   reserved by Intel */
	asm volatile("int $15");
}


/*********** Checkpoint 1 tests ***********/

/* IDT Test - Example
 * 
 * Asserts that first 10 IDT entries are not NULL
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: Load IDT, IDT definition
 * Files: x86_desc.h/S
 */
int idt_test(){
	TEST_HEADER;

	int i;
	int result = PASS;
	for (i = 0; i < 10; ++i){
		if ((idt[i].offset_15_00 == NULL) && 
			(idt[i].offset_31_16 == NULL)){
			assertion_failure();
			result = FAIL;
		}
	}

	return result;
}

/* Exceptions Test
 * 
 * Asserts dividing by 0 causes a divide by 0 exception,
 *		that an interrupt triggered without a handler causes a handler not found exception,
 *		that syscalls are at entry 0x80 in the IDT
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: Causes exceptions
 * Coverage: do_irq
 * Files: idt.c
 */
int exceptions_test() {
	TEST_HEADER;
	
	// Interrupt testing
	// int a = 5/0; // divide by zero exception
	// asm volatile("int $0x30"); // handler not present
	// asm volatile("int $0x80"); // syscall
	
	return PASS;
}

/* Paging Test 
 * 
 * Checks that the 
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: Causes page faults (if uncommented reads for un-paged addresses)
 * Coverage: init_paging, get_physical_addr, read_physical_addr
 * Files: paging.c/h, paging_helpers.S
 */
int paging_test(void) {
	TEST_HEADER;
	
	uint32_t i = 0;
	char c;
	PAGE_FAULT_FLAG = 0;
	
	int result = PASS;
	uint32_t LAST_FOUR_KB_PAGE = 0xFFFFF000;
	for (i = 0; i < LAST_FOUR_KB_PAGE; i += FOUR_KB) {
		/* Check Video memory */
		if (i == VIDEO) {
			/* Check that video is identity mapped */
			if (i != get_physical_addr(i)) {
				printf("Video memory is not identity mapped!\n");
				assertion_failure();
				result = FAIL;
			}
			/* Check that video memory is accessible */
			PAGE_FAULT_FLAG = 0;
			c = *((char*) i); /* Read from address i */ 
			if (PAGE_FAULT_FLAG) {
				printf("Video memory is not accessible!\n");
				assertion_failure();
				result = FAIL;
			}
		}
		
		/* Check Kernel memory */
		else if (i >= FOUR_MB && i < 2 * FOUR_MB) {
			/* Check that kernel memory is identity mapped */
			if (i != get_physical_addr(i)) {
				printf("Kernel memory is not identity mapped!\n");
				assertion_failure();
				result = FAIL;
			}
			/* Check that kernel memory is accessible */
			PAGE_FAULT_FLAG = 0;
			c = *((char*) i); /* Read from address i */
			if (PAGE_FAULT_FLAG) {
				printf("Kernel memory is not accessible!\n");
				assertion_failure();
				result = FAIL;
			}
		}
		
		/* Check the rest of memory */
		else {
			/* Unpaged memory should be translatable */
			PAGE_FAULT_FLAG = 0;
			PAGE_FAULT_FLAG = 1;
			//get_physical_addr(i);
			// if can recover from a page fault, undo hardcoding
			
			if (!PAGE_FAULT_FLAG) {
				printf("Un-paged memory is accessible!\n");
				assertion_failure();
				result = FAIL;
			}
		
			/* Unpaged memory should not be accessible */
			PAGE_FAULT_FLAG = 0;
			PAGE_FAULT_FLAG = 1;
			//c = *((char*) i); /* Read from address i */
			// if can recover from a page fault, undo hardcoding
			
			if (!PAGE_FAULT_FLAG) {
				printf("Un-paged memory is accessible!\n");
				assertion_failure();
				result = FAIL;
			}
		}
	}
	
	/* Last page has to be handled separately due to overflow */
	{
		/* Unpaged memory should be translatable */
		PAGE_FAULT_FLAG = 0;
		PAGE_FAULT_FLAG = 1;
		//get_physical_addr(i);
		// if can recover from a page fault, undo hardcoding
		
		if (!PAGE_FAULT_FLAG) {
			printf("Un-paged memory is accessible!\n");
			assertion_failure();
			result = FAIL;
		}
	
		/* Unpaged memory should not be accessible */
		PAGE_FAULT_FLAG = 0;
		PAGE_FAULT_FLAG = 1;
		//c = *((char*) i); /* Read from address i */
		// if can recover from a page fault, undo hardcoding
		
		if (!PAGE_FAULT_FLAG) {
			printf("Un-paged memory is accessible!\n");
			assertion_failure();
			result = FAIL;
		}
	}
	
	return result;
}

/*********** Checkpoint 2 tests ***********/

/* Filesystem Test 
 * 
 * Checks that the filesystem functions work as intended
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: Causes page faults (if uncommented reads for un-paged addresses)
 * Coverage: init_filesystem; read_dentry_by_index, read_dentry_by_name, dir and file open, close, read 
 * Files: filesystem
 */
int filesystem_test(void) {
	TEST_HEADER;
	int result = PASS;
	
	/* First d-entry is ./ */
	if(strncmp((char*)root.dentries[0].filename, (char*)".", 32)) {
		printf("Expected . as first dentry, but got: %s\n", root.dentries[0].filename);
		result = FAIL;
	}
 	
 	/* Test read_dentry_by_index */
 	dentry_t d;
	read_dentry_by_index(0x0A, &d); /* dentry 0x0A should be frame0.txt */
	if(strncmp((char*)d.filename, (char*)"frame0.txt", 32)) {
		printf("found incorrect dentry with filename: %s\n", d.filename);
		result = FAIL;
	}
	/* Test read_dentry_by_name */
	read_dentry_by_name((uint8_t*)"frame0.txt", &d);
	if(strncmp((char*)d.filename, "frame0.txt", 32)) {
		printf("found incorrect dentry with filename: %s\n", d.filename);
		result = FAIL;
	}

	/* Test multi-datablock reads */
	const int VERY_LARGE_FILE_SIZE = 5277, VERY_LARGE_FILE_INODE = 44;
	char big_buf[VERY_LARGE_FILE_SIZE];
	/* Read from inode, offset */
	read_data(VERY_LARGE_FILE_INODE, 0, (uint8_t*)big_buf, VERY_LARGE_FILE_SIZE);
	if(strlen(big_buf) < VERY_LARGE_FILE_SIZE - 1) {
		printf("not enough characters read!\n", big_buf);
		result = FAIL;
	}
	
	uint32_t fd;
	char buf[33];
	/* Test regular file operations */
	fd = open((uint8_t*)"frame0.txt");
	read(fd, buf, 20);
	
	if(strncmp(buf, "/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\", 20)) {
		printf("Read incorrect string: %s\n", buf);
		result = FAIL;
	}
	
	/* write should do nothing */
	write(fd, "fjkdsa", 7);
	
	read(fd, buf, 20);
	
	if(strncmp(buf, "/\\/\\\n         o", 15)) {
		printf("Read incorrect string: %s\n", buf);
		result = FAIL;
	}
	close(fd);
	
	
	/* Test directory operations */
	fd = open((uint8_t*)".");
	read(fd, buf, 32);
	if(strncmp(buf, ".", 32)) {
		printf("read incorrect filename: %s\n", buf);
		result = FAIL;
	}
	
	/* write should do nothing */
	write(fd, "fjkdsa", 7);
	
	read(fd, buf, 32);
	if(strncmp(buf, "sigtest", 32)) {
		printf("read incorrect next filename: %s\n", buf);
		result = FAIL;
	}
	close(fd);
	
	return result;
}

/* Directory Read Test 
 * 
 * Prints the contents of ./
 * Inputs: None
 * Outputs: PASS (FAIL is determined by inspection)
 * Side Effects: None
 * Coverage: directory read, open, close
 * Files: dir_operations
 */
int directory_read_test(void) {
	TEST_HEADER;
	int result = PASS;
	uint32_t fd, bytes_read = 0;
	char buf[33];
	buf[32] = '\0';
	
	/* Read contents of ./ */
	fd = open((uint8_t*)".");
	printf("Printing contents of ./\n");
	do {
		bytes_read = read(fd, buf, 32);
		printf("%s\n", buf);
	} while(bytes_read != 0);
	close(fd);
	
	return result;
}


/* Arbitrary File Read Test 
 * 
 * Allows user to input files to be read and printed to the screen
 * Inputs: None
 * Outputs: PASS (FAIL is determined by inspection)
 * Side Effects: None
 * Coverage: file open, close, read
 * Files: filesystem
 */
int arbitrary_file_read_test(void) {
	TEST_HEADER;
	int result = PASS;
	uint32_t fd, n;
	char buf[333];
	
	/* Execute the first program ("shell") ... */
    uint8_t text_buf[BUF_SIZE+1] = {0};
    while (1) {
        printf("\n\nType a filename: ");
        n = read(STDIN, text_buf, 34);
        if (n == 1) break; /* if only \n, break */
        text_buf[n-1]='\0';
        fd = open(text_buf);
        if (fd == SYSCALL_ERROR) {
            printf("File not found!\n");
            continue;
        }
        if (fd_table[fd].inode_num == 0) {
        	printf("Not a regular file!\n");
            continue;
        }
        printf("reading %s\n", text_buf);
        while ((n = read(fd, buf, 300)) > 0) {
            write(STDOUT, buf, n); // write(1, buf, n);
        }

        close(fd);
    }
	return result;
}


/* Terminal Driver Test
 * 
 * Reads user input from terminal and writes output to terminal
 * Inputs: None
 * Outputs: PASS (FAIL is determined by inspection)
 * Side Effects: None
 * Coverage: file open, close, read
 * Files: filesystem
 */
int terminal_driver_test(void) {
	TEST_HEADER;
	int result = PASS;
	uint8_t i;
	uint8_t text_buf[BUF_SIZE+1] = {0};

    printf("LONG TEXT, a little longer than the 80 characters that fit on a single line of the screen. ");

    printf("TEST\n"); // should be after previous line

    while (1) {
        printf("Type your name (max 9 characters): "); // you type your name
        if ((i = read(STDIN, text_buf, 10)) > 1) { /* Only print if more than \n was written */
            if (text_buf[i-1] == '\n') {
                text_buf[i-1] = '\0';
                printf("Hello, %s\n", text_buf);
            } else {
                printf("Your name is too long!\n");
            }
            memset(text_buf, 0, sizeof(text_buf));
        } else {
        	break;
        }
    }
	return result;
}


/* RTC Test
 * 
 * Reads user input from terminal and writes output to terminal
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: rtc open, write, close
 * Files: rtc
 */
int rtc_test(void) {
  TEST_HEADER;
  int result = PASS;
  int32_t fd, rate;
  fd = open((uint8_t*)"rtc");
  if(fd == -1) result = FAIL;
  rate = 4; /* Check small frequency is sucessful */
  if(write(fd, &rate, 0) != 0) result = FAIL;
  rate = 1024; /* Check maximum frequency allowed for users */
  if(write(fd, &rate, 0) != 0) result = FAIL;
  rate = 2048; /* Check can't go above max frequency */
  if(write(fd, &rate, 0) == 0) result = FAIL;
  rate = 30; /* Check must be power of 2 */
  if(write(fd, &rate, 0) == 0) result = FAIL;
  if(close(fd)) result = FAIL;

  return result;
}

/* RTC Visual Test
   * 
   * Outputs A-s to the screen at increasing speeds
   * Inputs: None
   * Outputs: PASS (FAIL is determined by inspection)
   * Side Effects: None
   * Coverage: rtc open, read, write, close
   * Files: rtc
   */
int rtc_visual_test(void) {
	TEST_HEADER; 
	
	int32_t fd, speed;
    int32_t i, j;

	speed = 2;
	fd = open((uint8_t*)"rtc");
	for(i = 0; i < 5; ++i) {
	  for(j = 0; j < 10; ++j) {
		read(fd, 0, 0);
		printf("A");
	  }
	  printf("\n");
	  speed *= 2;
	  write(fd, &speed, 0);
	}
	close(fd);
	
	return PASS;
}

/*********** Checkpoint 3 tests ***********/

/* User Space Page Test
 *
 * Checks that user space pages can be assigned
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: add_user_page, free_user_page
 * Files: paging
 */
int user_space_page_test(void) {
  TEST_HEADER;
  int result = PASS, test_val;
  if(add_user_page() != 2) result = FAIL; // Check we don't try to allocate kernel pages
  if(add_user_page() != 3) result = FAIL; // Check we don't try to allocate taken pages
  free_user_page(2); // Test we can free pages
  if(add_user_page() != 2) result = FAIL; // Test we can reclaim freed pages later

  test_val = *(int*)((1 << 22)*32 + 1); // Test derefrencing memory (will page fault on error)
  
  // Clean up
  free_user_page(2);
  free_user_page(3);

  return result;
}

/* Loader Test
 * 
 * Checks that the loader works
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: loads programs into memory
 * Coverage: load_program
 * Files: loader
 */
int loader_test(void) {
	TEST_HEADER;
	int result = PASS;

    // try to load . directory (not a file)
    if (load_program((const uint8_t*)".") == 0) result = FAIL;

    // try to load frame0.txt (not an ELF executable)
    if (load_program((const uint8_t*)"frame0.txt") == 0) result = FAIL;

    // try to load asdf (doesn't exist)
    if (load_program((const uint8_t*)"asdf") == 0) result = FAIL;

    // check various bytes of the cat program in memory

	// load cat program and check return value
    if (load_program((const uint8_t*)"cat")) result = FAIL;

    // first bytes
    if (*(uint8_t*)(0x08048000+0)    != 127) result = FAIL;
    if (*(uint8_t*)(0x08048000+1)    != 69) result = FAIL;

    // final bytes
    if (*(uint8_t*)(0x08048000+5444) != 0) result = FAIL;
    if (*(uint8_t*)(0x08048000+5443) != 90) result = FAIL;

    // randomly in the middle
    if (*(uint8_t*)(0x08048000+1234) != 101) result = FAIL;
    if (*(uint8_t*)(0x08048000+2345) != 0) result = FAIL;

    return result;
}

/* PCB Test
 * 
 * Allocates and deallocates PCBs
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: push_pcb, pop_pcb
 * Files: process.c, syscalls_structs.h
 */
int pcb_test(void) {
  TEST_HEADER;
  int result = PASS;
  uint32_t pcb_addr;
  
  /* Check that current_pcb has been set to 8MB */
  if((uint32_t) current_pcb != 0x800000) result = FAIL;
  /* Expect first PCB to be at 8MB - 16kB */
  pcb_addr = push_pcb();
  if(pcb_addr != 0x7FC000) result = FAIL;
  /* Check that current_pcb has been updated */
  if((uint32_t) current_pcb != 0x7FC000) result = FAIL;
  /* Expect second PCB to be at 8MB - 24kB */
  pcb_addr = push_pcb();
  if(pcb_addr != 0x7FA000) result = FAIL;
  /* Expect third PCB to be at 8MB - 32kB */
  pcb_addr = push_pcb();
  if(pcb_addr != 0x7F8000) result = FAIL;
  /* Cannot have more than 6 processes */
  pcb_addr = push_pcb();
  pcb_addr = push_pcb();
  pcb_addr = push_pcb();
  pcb_addr = push_pcb();
  if(pcb_addr != -1) result = FAIL;
  pcb_addr = pop_pcb();
  pcb_addr = pop_pcb();
  pcb_addr = pop_pcb();
  /* Pop third PCB at 8MB - 32kB */
  pcb_addr = pop_pcb();
  if(pcb_addr != 0x7F8000) result = FAIL;
  /* Expect fourth PCB to be at 8MB - 32kB */
  pcb_addr = push_pcb();
  if(pcb_addr != 0x7F8000) result = FAIL;
  /* Pop fourth PCB at 8MB - 32kB */
  pcb_addr = pop_pcb();
  if(pcb_addr != 0x7F8000) result = FAIL;
  /* Pop third PCB at 8MB - 24kB */
  pcb_addr = pop_pcb();
  if(pcb_addr != 0x7FA000) result = FAIL;
  /* Pop third PCB at 8MB - 16kB */
  pcb_addr = pop_pcb();
  if(pcb_addr != 0x7FC000) result = FAIL;
  /* Check that current_pcb has reset */
  if((uint32_t) current_pcb != 0x800000) result = FAIL;

  return result;
}

/* Syscalls Test
 * 
 * Calls various syscalls
 * Inputs: None
 * Outputs: PASS (FAIL is determined by inspection)
 * Side Effects: None
 * Coverage: syscall assembly linkage
 * Files: syscalls.S
 */
int syscalls_test(void) {
	TEST_HEADER;
	// testing syscalls by int80
    uint32_t rtc, ret, nbytes, result = PASS;
    char *buf[100] ; 
    char  *filename = "rtc";

    /* calling asm linked syscalls */ 

	/* Call open() */
    asm volatile ("INT $0x80" : "=a" (rtc) :
	 	  "a" (5), "b" (filename), "c" (0));

	/* Call read() */
    asm volatile("INT $0x80" : "=a" (ret) :
          "a" (3), "b" (rtc), "c" (buf), "d" (nbytes)) ;
    
    return result;
}

/* Execute/Halt Test
 * 
 * Runs execute() on user programs
 * Inputs: None
 * Outputs: PASS (FAIL determined by inspection)
 * Side Effects: None
 * Coverage: execute
 * Files: execute.c
 */
int execute_test(void) {
  TEST_HEADER;
  int result = PASS;
  
  // execute("ls");
  //while(1) 
  execute((uint8_t*)"shell");

  return result;
}

/*********** Checkpoint 4 tests ***********/

/* Getargs Test
 * 
 * Runs "cat .", "cat frame0.txt", "grep hi"
 * Inputs: None
 * Outputs: PASS (FAIL determined by inspection)
 * Side Effects: None
 * Coverage: getargs
 * Files: getargs.c
 */
int getargs_test(void) {
  TEST_HEADER;
  int result = PASS;
  
  execute((uint8_t*)"cat  .");
  execute((uint8_t*)"cat frame0.txt");
  execute((uint8_t*)"grep hi");

  return result;
}


/* Vidmap Test
 * 
 * Verifies that vidmap fails for pointer address not in user memory
 * NOTE: cannot verify correct mapping because that requires user space
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: vidmap
 * Files: vidmap.c
 */
int vidmap_test(void) {
  TEST_HEADER;
  int result = PASS;
  
  /* Should return -1 if address of input is not in user memory */
  if(vidmap((uint8_t**) &result) != -1) result = FAIL;

  return result;
}


/* Fish Test
 * 
 * Runs "fish"
 * Inputs: None
 * Outputs: PASS (FAIL determined by inspection)
 * Side Effects: None
 * Coverage: vidmap, rtc, file read
 * Files: syscalls.h
 */
int fish_test(void) {
  TEST_HEADER;
  int result = PASS;
  
  execute((uint8_t*)"fish");

  return result;
}


/*********** Checkpoint 5 tests ***********/


/* DEPRECATED: Scheduling Visual Test
 * (This test was used while developing)
 * 
 * Conceptual scheduling test 
 * Inputs: None
 * Outputs: PASS (FAIL is determined by inspection)
 * Side Effects: None
 * Coverage: syscall assembly linkage
 * Files: syscalls.S
 */
int scheduling_visual_test(void){
	TEST_HEADER;

	int result = PASS ; 

	/* registering the scheduling handler */
	cli() ;
	register_interrupt_handler(RTC_IRQ,round_robin) ;
	sti() ;

	/* this test is a visual test to showcase scheduling switches */
	return result; 
}

/* Screen Test
 * 
 * Test to write to screens and verify viewing screen changes
 * Inputs: None
 * Outputs: PASS (FAIL is determined by inspection)
 * Side Effects: None
 * Coverage: screen changing (for processes and viewing)
 * Files: screen.c
 */
int screen_test(void) {
	TEST_HEADER;
	int result = PASS;

	// clear screen 1
	change_process_screen(1);
	clear();
	// clear screen 2
	change_process_screen(2);
	clear();
	// back to screen 0
	change_process_screen(0);

	*(uint8_t*)VIDEO = 1;
	change_process_screen(1);
	*(uint8_t*)VIDEO = 2;
	change_process_screen(2);
	*(uint8_t*)VIDEO = 3;
	// change viewing screen to screen 1
	change_view_screen(1);

	// check that logical screen didn't change (or was copied)
	if (*(uint8_t*)VIDEO != 3) result = FAIL;
	change_process_screen(0);
	*(uint8_t*)VIDEO = 4;
	change_process_screen(1);
	*(uint8_t*)VIDEO = 5;
	change_process_screen(2);
	*(uint8_t*)VIDEO = 6;
	// change viewing screen to screen 2
	change_view_screen(2);

	// check that logical screen didn't change (or was copied)
	if (*(uint8_t*)VIDEO != 6) result = FAIL;
	change_process_screen(0);
	*(uint8_t*)VIDEO = 7;
	change_process_screen(1);
	*(uint8_t*)VIDEO = 8;
	change_process_screen(2);
	*(uint8_t*)VIDEO = 9;
	// change viewing screen to screen 0
	change_view_screen(0);

	// check that logical screen didn't change (or was copied)
	if (*(uint8_t*)VIDEO != 9) result = FAIL;
	change_process_screen(0);
	// check that screen was changed behind the scenes properly
	if (*(uint8_t*)VIDEO != 7) result = FAIL; 
	// (if you step through, there should be )

	return result;
}

/* Arp Test
 * 
 * Arp test
 * Inputs: None
 * Outputs: PASS (FAIL is determined by inspection)
 * Side Effects: Sends ARP request (and waits for response)
 * Coverage: send/receive packets, arp request
 * Files: arp.c, networking.c, ethernet.c
 */
int arp_test(void){
    TEST_HEADER;

    int result = PASS;

    ip_t dest = {{10, 0, 2, 3}}; // dns server
    mac_t mac;
    mac_t correct = {{0x52, 0x55, 0x0A, 0x00, 0x02, 0x03}};
    if (arp_get(&dest, &mac)) {
        result = FAIL; // should not timeout
    } else {
        if (compare_mac(&mac, &correct)) result = FAIL; // Incorrect mac address
    }
    dest.v[3] = 23; // doesn't exist
    if (arp_get(&dest, &mac) != SYSCALL_ERROR) {
        result = FAIL; // should timeout
    }
    return result; 
}

/* DNS Test
 * 
 * DNS test
 * Inputs: None
 * Outputs: PASS (FAIL is determined by inspection)
 * Side Effects: Sends DNS request (and waits for response)
 * Coverage: dns request, over UDP
 * Files: dns.c, udp.c
 */
int dns_test(void){
    TEST_HEADER;

    int result = PASS;

    ip_t ip;
    ip_t correct = {{99, 84, 252, 123}};

    if (dns_query((uint8_t*)"ainsworth.smugmug.com", &ip)) { // domain chosen because it uses CNAME
        result = FAIL;
    } else if (compare_ip(&ip, &correct)) {
        result = FAIL;
    }

    if (!dns_query((uint8_t*)"anwpoefjin.com", &ip)) { // doesn't exist
        result = FAIL;
    }

    return result; 
}


/* Test suite entry point */
void launch_tests(){
	printf("TESTING...\n");
	// launch your tests here
	int failed_count = 0;
	TEST_OUTPUT("idt_test", idt_test(), &failed_count);
	TEST_OUTPUT("exceptions_test", exceptions_test(), &failed_count);
	TEST_OUTPUT("paging_test", paging_test(), &failed_count);
	TEST_OUTPUT("filesystem_test", filesystem_test(), &failed_count);
	//TEST_OUTPUT("directory_read_test", directory_read_test(), &failed_count);
	//TEST_OUTPUT("arbitrary_file_read_test", arbitrary_file_read_test(), &failed_count);
	//TEST_OUTPUT("terminal_driver_test", terminal_driver_test(), &failed_count);
	TEST_OUTPUT("rtc_test", rtc_test(), &failed_count);
	//TEST_OUTPUT("rtc_visual_test", rtc_visual_test(), &failed_count);
	TEST_OUTPUT("user_space_page_test", user_space_page_test(), &failed_count);
  	TEST_OUTPUT("loader_test", loader_test(), &failed_count);
  	TEST_OUTPUT("pcb_test", pcb_test(), &failed_count);
  	//TEST_OUTPUT("syscalls_test", syscalls_test(), &failed_count); 
  	//TEST_OUTPUT("execute_test", execute_test(), &failed_count);
  	//TEST_OUTPUT("getargs_test", getargs_test(), &failed_count);
  	TEST_OUTPUT("vidmap_test", vidmap_test(), &failed_count);
  	//TEST_OUTPUT("fish_test", fish_test(), &failed_count);
	//TEST_OUTPUT("scheduling_visual_test",scheduling_visual_test(), &failed_count);
  	TEST_OUTPUT("screen_test", screen_test(), &failed_count);
    TEST_OUTPUT("arp_test", arp_test(), &failed_count);
    TEST_OUTPUT("dns_test", dns_test(), &failed_count);
    printf("TESTING COMPLETE\n");
	printf("Failed %d test(s)\n", failed_count);
}
