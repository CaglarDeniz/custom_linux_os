/* paging.h - Defines structs for pdes and ptes
 * vim:ts=4 noexpandtab
 */

#ifndef PAGING_H
#define PAGING_H

#include "lib.h"

/* Kernel memory goes from 4-8 MB */
#define KERNEL_MEM_END EIGHT_MB

/* User Virtual memory goes from 128-132 MB */
#define RESERVED_MEM_PAGE_INDEX 0
#define KERNEL_MEM_PAGE_INDEX 1
#define USER_MEM_PAGE_INDEX 32

/* Assign User Video Memory Map to Virtual Address 0x4000000 (64 MB) */
#define VIDMAP_MEM_ADDR 0x4000000
#define VIDMAP_MEM_PAGE_INDEX (VIDMAP_MEM_ADDR / FOUR_MB) 
#define VIDMAP_PAGE_TABLE_INDEX 0

#define FOUR_KB_PAGE_SIZE FOUR_KB

#define NUM_BYTES_PER_PAGE_DIR_ENTRY 4
#define NUM_BYTES_PER_PAGE_TABLE_ENTRY 4

#define NUM_PAGE_DIR_ENTRIES (FOUR_KB_PAGE_SIZE / NUM_BYTES_PER_PAGE_DIR_ENTRY) /* Should be 4GB / 4MB */
#define NUM_PAGE_TABLE_ENTRIES (FOUR_KB_PAGE_SIZE / NUM_BYTES_PER_PAGE_TABLE_ENTRY) /* Should be 4MB / 4KB */

#ifndef ASM

typedef union page_directory_entry {
    uint32_t val;
    struct { /* Page directory entry for 4 kB page (points to page table) */
        uint32_t present 				: 1;	/* 1 if valid, 0 if does not exist */
        uint32_t read_write_perm 		: 1;	/* 1 if r/w, 0 for read-only */
        uint32_t user_super 			: 1;	/* 1 for user, 0 for kernel-only */
        uint32_t page_write_through 	: 1;	/* if cached, 0 for write-back, 
        										1 for write-through, for MP3 always 0 */
        uint32_t page_cache_disabled 	: 1;	/* 1 for no caching 
        										(ex. memory mapped io like video mem) */
		uint32_t accessed 				: 1;	/* if accessed */
		uint32_t zero					: 1;	/* always 0 */
		uint32_t page_size 				: 1;	/* 1 for 4 MB, (0 for 4 kB PT) */
		uint32_t global 				: 1;	/* if used by multiple processes*/
		uint32_t avail 					: 3;	/* Usable, but unused */
		uint32_t page_table_base_addr 	: 20; 	/* Base addr for PT */
    } __attribute__ ((packed));
    struct { /* Page directory entry for 4 MB page */
        uint32_t present_				: 1;	/* 1 if valid, 0 if does not exist */
        uint32_t read_write_perm_		: 1;	/* 1 if r/w, 0 for read-only */
        uint32_t user_super_			: 1;	/* 1 for user, 0 for kernel-only */
        uint32_t page_write_through_	: 1;	/* if cached, 0 for write-back, 
        										1 for write-through, for MP3 always 0 */
        uint32_t page_cache_disabled_	: 1;	/* 1 for no caching 
        										(ex. memory mapped io like video mem) */
		uint32_t accessed_				: 1;	/* if accessed */
		uint32_t dirty					: 1;	/* Set(1) by processor if written to,
        										cleared by user, unused by MP3, set to 0 */
		uint32_t page_size_				: 1;	/* (1 for 4 MB), 0 for 4 kB PT */
		uint32_t global_				: 1;	/* if used by multiple processes*/
		uint32_t avail_					: 3;	/* Usable, but unused */
		uint32_t page_attr_table_idx	: 1;	/* Unused for MP3, set to 0 */
		uint32_t reserved				: 9;	/* MUST be set to 0 or 
        										invalid PDE exception */
		uint32_t page_base_addr		 	: 10; 	/* Base addr for 4 MB page */
    } __attribute__ ((packed));
} pde_t;



typedef union page_table_entry {
    uint32_t val;
    struct { /* Page directory entry for 4 kB page (points to page table) */
        uint32_t present 				: 1;	/* 1 if valid, 0 if does not exist */
        uint32_t read_write_perm 		: 1;	/* 1 if r/w, 0 for read-only */
        uint32_t user_super 			: 1;	/* 1 for user, 0 for kernel-only */
        uint32_t page_write_through 	: 1;	/* if cached, 0 for write-back, 
        										1 for write-through, for MP3 always 0 */
		uint32_t page_cache_disabled 	: 1;	/* 1 for no caching 
        										(ex. memory mapped io like video mem) */
		uint32_t accessed 				: 1;	/* if accessed */
		uint32_t dirty					: 1;	/* Set(1) by processor if written to,
        										cleared by user, unused by MP3, set to 0 */
		uint32_t page_attr_table_idx 	: 1;	/* Unused for MP3, set to 0 */
		uint32_t global 				: 1;	/* if used by multiple processes*/
		uint32_t avail 					: 3;	/* Usable, but unused */
		uint32_t page_base_addr 		: 20; 	/* Base addr for 4 kB page */
    } __attribute__ ((packed));
} pte_t;

typedef union {
	uint32_t addr;
	struct {
		uint32_t offset_four_mb	: 22;
		uint32_t page_dir_entry	: 10;
	} __attribute__ ((packed));
	struct {
		uint32_t offset_four_kb		: 12;
		uint32_t page_table_entry	: 10;
		uint32_t page_dir_entry_	: 10;
	} __attribute__ ((packed));
} virtual_addr_t;


pde_t page_directory[NUM_PAGE_DIR_ENTRIES] __attribute__((aligned(FOUR_KB)));
pte_t page_table0[NUM_PAGE_TABLE_ENTRIES] __attribute__((aligned(FOUR_KB)));
pte_t page_table_vidmap[NUM_PAGE_TABLE_ENTRIES] __attribute__((aligned(FOUR_KB)));

#define PAGE_TABLE0_BASE_ADDR (((uint32_t) (page_table0) / FOUR_KB) & 0x000FFFFF)
#define PAGE_TABLE_VIDMAP_BASE_ADDR (((uint32_t) (page_table_vidmap) / FOUR_KB) & 0x000FFFFF)


/* Sets the control registers to enable paging */
extern void set_control_regs(pde_t* page_directory_ptr);

/* Initializes paging */
extern void paging_init(void);

/* Converts a virutal memory address into a physical memory address (we are skipping linear addresses) */
extern uint32_t get_physical_addr(uint32_t virtual_addr);

/* Flushes TLB */
extern void flush_tlb(void);

/* User program paging */
extern uint32_t add_user_page(void);
extern void free_user_page(uint32_t page_num);
extern void set_user_page(uint32_t page_num);
 
/* Vidmap enable/disable */
extern uint32_t enable_vidmap(void);
extern uint32_t disable_vidmap(void);

/* Screen mapping functions */
void map_video_to_video(void);
void map_vidmap_to_video(void);
void map_video_to_backup(int n);
void map_vidmap_to_backup(int n);

void brute_add_page(uint32_t addr);

#endif /* ASM */

#endif /* PAGING_H */

