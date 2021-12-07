/* paging.c - Initializes paging and converts virtual addresses to physical addresses
 * vim:ts=4 noexpandtab
 */

#include "lib.h"
#include "paging.h"

#define VIDEO_MEM_PAGE_INDEX (VIDEO / FOUR_KB_PAGE_SIZE)

int num_vidmaps = 0;

/* void paging_init(void);
 * Inputs: void
 * Return Value: none
 * Function: Initializes paging with entries for 4 MB page kernel memory 
 *				and a 4 kB page for Video memory */
void paging_init(void) {
	memset(page_directory, 0, NUM_PAGE_DIR_ENTRIES * sizeof(pde_t));

	/*** Set PDE for kernel memory (4 MB) @ 4-8 MB ***/
	page_directory[KERNEL_MEM_PAGE_INDEX].page_base_addr = 1;		/* 1st block from 4 MB to 8MB */
	page_directory[KERNEL_MEM_PAGE_INDEX].reserved = 0x000;			/* MUST be set to 0 */
	page_directory[KERNEL_MEM_PAGE_INDEX].page_attr_table_idx = 0;	/* Set to 0 for MP3 */
	page_directory[KERNEL_MEM_PAGE_INDEX].avail = 0;				/* Unused */
	page_directory[KERNEL_MEM_PAGE_INDEX].global = 1;				/* Kernel memory should be global */
	page_directory[KERNEL_MEM_PAGE_INDEX].page_size = 1;			/* Is a 4 MB page */
	page_directory[KERNEL_MEM_PAGE_INDEX].dirty = 0;				/* Not dirty */
	page_directory[KERNEL_MEM_PAGE_INDEX].accessed = 0;				/* Has not been accessed */
	page_directory[KERNEL_MEM_PAGE_INDEX].page_cache_disabled = 0;	/* Allow page caching */
	page_directory[KERNEL_MEM_PAGE_INDEX].page_write_through = 0; 	/* For MP3, always write-back */
	page_directory[KERNEL_MEM_PAGE_INDEX].user_super = 0;			/* Kernel-only memory */
	page_directory[KERNEL_MEM_PAGE_INDEX].read_write_perm = 1;		/* Allow read/write for kernel */
	page_directory[KERNEL_MEM_PAGE_INDEX].present = 1;				/* Page is being used */
	
	/*** Set PDE and PTE for Video Memory (PT, PTE) ***/
	page_directory[RESERVED_MEM_PAGE_INDEX].page_table_base_addr 
		= PAGE_TABLE0_BASE_ADDR;						/* get 20 MSB of physical address of page table */
	page_directory[RESERVED_MEM_PAGE_INDEX].avail = 0;					/* Unused */
	page_directory[RESERVED_MEM_PAGE_INDEX].global = 0;					/* Reserved memory should not be global */
	page_directory[RESERVED_MEM_PAGE_INDEX].page_size = 0;				/* Is for 4 kB pages */
	page_directory[RESERVED_MEM_PAGE_INDEX].zero = 0;					/* Always 0 */
	page_directory[RESERVED_MEM_PAGE_INDEX].accessed = 0;				/* Has not been accessed */
	page_directory[RESERVED_MEM_PAGE_INDEX].page_cache_disabled = 0;	/* Allow page caching */
	page_directory[RESERVED_MEM_PAGE_INDEX].page_write_through = 0; 	/* For MP3, always write-back */
	page_directory[RESERVED_MEM_PAGE_INDEX].user_super = 0;				/* Kernel-only memory (no users) */
	page_directory[RESERVED_MEM_PAGE_INDEX].read_write_perm = 0;		/* Disallow write */
	page_directory[RESERVED_MEM_PAGE_INDEX].present = 1;				/* Page is being used */

	/* PTE for Video memory */
	memset(page_table0, 0, NUM_PAGE_TABLE_ENTRIES * sizeof(pte_t));
	page_table0[VIDEO_MEM_PAGE_INDEX].page_base_addr = VIDEO_MEM_PAGE_INDEX; /* Address of video memory (0xB8000) */
	page_table0[VIDEO_MEM_PAGE_INDEX].avail = 0;					/* Unused */
	page_table0[VIDEO_MEM_PAGE_INDEX].global = 0;					/* Video memory should not be global */
	page_table0[VIDEO_MEM_PAGE_INDEX].page_attr_table_idx = 0;		/* Unused for MP3, set to 0 */
	page_table0[VIDEO_MEM_PAGE_INDEX].dirty = 0;					/* Not dirty */
	page_table0[VIDEO_MEM_PAGE_INDEX].accessed = 0;					/* Has not been accessed */
	page_table0[VIDEO_MEM_PAGE_INDEX].page_cache_disabled = 1;		/* Disable page caching */
	page_table0[VIDEO_MEM_PAGE_INDEX].page_write_through = 0; 		/* For MP3, always write-back */
	page_table0[VIDEO_MEM_PAGE_INDEX].user_super = 0;				/* Kernel-only memory (no users) */
	page_table0[VIDEO_MEM_PAGE_INDEX].read_write_perm = 1;			/* Allow write for kernel */
	page_table0[VIDEO_MEM_PAGE_INDEX].present = 1;					/* Page is being used */

	/* PDE for Userspace */
	page_directory[USER_MEM_PAGE_INDEX].page_base_addr = 2;			/* Starts at 8MB */
	page_directory[USER_MEM_PAGE_INDEX].reserved = 0x000;			/* MUST be set to 0 */
	page_directory[USER_MEM_PAGE_INDEX].page_attr_table_idx = 0;	/* Set to 0 for MP3 */
	page_directory[USER_MEM_PAGE_INDEX].avail = 0;					/* Unused */
	page_directory[USER_MEM_PAGE_INDEX].global = 0;					/* Kernel memory should be global */
	page_directory[USER_MEM_PAGE_INDEX].page_size = 1;				/* Is a 4 kB page */
	page_directory[USER_MEM_PAGE_INDEX].dirty = 0;					/* Not dirty */
	page_directory[USER_MEM_PAGE_INDEX].accessed = 0;				/* Has not been accessed */
	page_directory[USER_MEM_PAGE_INDEX].page_cache_disabled = 0;	/* Allow page caching */
	page_directory[USER_MEM_PAGE_INDEX].page_write_through = 0; 	/* For MP3, always write-back */
	page_directory[USER_MEM_PAGE_INDEX].user_super = 1;				/* Kernel-only memory */
	page_directory[USER_MEM_PAGE_INDEX].read_write_perm = 1;		/* Allow read/write */
	page_directory[USER_MEM_PAGE_INDEX].present = 1;				/* Page is being used */
	
	
	/* PT (PDE) for Vidmap */
	page_directory[VIDMAP_MEM_PAGE_INDEX].page_table_base_addr 
		= PAGE_TABLE_VIDMAP_BASE_ADDR;								/* get 20 MSB of physical address of page table */
	page_directory[VIDMAP_MEM_PAGE_INDEX].avail = 0;				/* Unused */
	page_directory[VIDMAP_MEM_PAGE_INDEX].global = 0;				/* Do not cache this memory */
	page_directory[VIDMAP_MEM_PAGE_INDEX].page_size = 0;			/* Is for 4 kB pages */
	page_directory[VIDMAP_MEM_PAGE_INDEX].zero = 0;					/* Always 0 */
	page_directory[VIDMAP_MEM_PAGE_INDEX].accessed = 0;				/* Has not been accessed */
	page_directory[VIDMAP_MEM_PAGE_INDEX].page_cache_disabled = 0;	/* Allow page caching */
	page_directory[VIDMAP_MEM_PAGE_INDEX].page_write_through = 0; 	/* For MP3, always write-back */
	page_directory[VIDMAP_MEM_PAGE_INDEX].user_super = 1;			/* Kernel-only memory (no users) */
	page_directory[VIDMAP_MEM_PAGE_INDEX].read_write_perm = 1;		/* Allow write */
	page_directory[VIDMAP_MEM_PAGE_INDEX].present = 1;				/* Page Table is being used 
																		(but no 4 kB pages are used on init) */

	/* PTE for Vidmap memory */
	memset(page_table_vidmap, 0, NUM_PAGE_TABLE_ENTRIES * sizeof(pte_t));
	page_table_vidmap[VIDMAP_PAGE_TABLE_INDEX].page_base_addr 
		= VIDEO_MEM_PAGE_INDEX; 										/* Address of video memory (0xB8000) */
	page_table_vidmap[VIDMAP_PAGE_TABLE_INDEX].avail = 0;					/* Unused */
	page_table_vidmap[VIDMAP_PAGE_TABLE_INDEX].global = 0;					/* Video memory should not be global */
	page_table_vidmap[VIDMAP_PAGE_TABLE_INDEX].page_attr_table_idx = 0;	/* Unused for MP3, set to 0 */
	page_table_vidmap[VIDMAP_PAGE_TABLE_INDEX].dirty = 0;				/* Not dirty */
	page_table_vidmap[VIDMAP_PAGE_TABLE_INDEX].accessed = 0;			/* Has not been accessed */
	page_table_vidmap[VIDMAP_PAGE_TABLE_INDEX].page_cache_disabled = 1;	/* Disable page caching */
	page_table_vidmap[VIDMAP_PAGE_TABLE_INDEX].page_write_through = 0; 	/* For MP3, always write-back */
	page_table_vidmap[VIDMAP_PAGE_TABLE_INDEX].user_super = 1;			/* User accessible memory */
	page_table_vidmap[VIDMAP_PAGE_TABLE_INDEX].read_write_perm = 1;		/* Allow write */
	page_table_vidmap[VIDMAP_PAGE_TABLE_INDEX].present = 0;				/* Page is not currently being used */

	/* Set CR0, CR3, CR4 to enable paging */
	set_control_regs(page_directory);
}

/* NOTE: this function is only used by tests.c since paging is done by the hardware
 * uint32_t get_physical_addr(uint32_t virtual_addr);
 * Inputs: virtual_addr - virtual address to be 
 *			converted into a physical address
 * Return Value: physical address corresponding 
 *			with the virtual address input
 * Function: Converts a virtual address to a physical address 
 *			using the Page Directory and its entries */
uint32_t get_physical_addr(uint32_t virtual_addr) {
	virtual_addr_t addr = { .addr = virtual_addr };

	if (!page_directory[addr.page_dir_entry].present) {
		asm volatile("int $0xE"); /* Page fault */
		return 0;
	}
	
	if (page_directory[addr.page_dir_entry].page_size) {
		/* Is 4 MB page*/
		return (page_directory[addr.page_dir_entry].page_base_addr * FOUR_MB) + addr.offset_four_mb;
	}
	else {
		/* Is PT of 4 kB pages */
		
		if (page_directory[addr.page_dir_entry].page_table_base_addr != PAGE_TABLE0_BASE_ADDR) {
			asm volatile("int $0xE"); /* Page fault */
			return 0;
		}
		
		if (!page_table0[addr.page_table_entry].present) {
			asm volatile("int $0xE"); /* Page fault */
			return 0;
		}
		return (page_table0[addr.page_table_entry].page_base_addr * FOUR_KB) + addr.offset_four_kb;
	}
}

// TODO
void brute_add_page(uint32_t addr) {
	// basically add page like kernel page
	uint32_t idx = addr / FOUR_MB;
	page_directory[idx].page_base_addr = idx;		/* 1st block from 4 MB to 8MB */
	page_directory[idx].reserved = 0x000;			/* MUST be set to 0 */
	page_directory[idx].page_attr_table_idx = 0;	/* Set to 0 for MP3 */
	page_directory[idx].avail = 0;				/* Unused */
	page_directory[idx].global = 1;				/* Kernel memory should be global */
	page_directory[idx].page_size = 1;			/* Is a 4 MB page */
	page_directory[idx].dirty = 0;				/* Not dirty */
	page_directory[idx].accessed = 0;				/* Has not been accessed */
	page_directory[idx].page_cache_disabled = 0;	/* Allow page caching */
	page_directory[idx].page_write_through = 0; 	/* For MP3, always write-back */
	page_directory[idx].user_super = 0;			/* Kernel-only memory */
	page_directory[idx].read_write_perm = 1;		/* Allow read/write for kernel */
	page_directory[idx].present = 1;				/* Page is being used */

	flush_tlb();
}

/* Note: need to skip the first two 4MB blocks since those are already used */
uint32_t user_pages = 0x3; // Bitmap to keep track of free pages

/* uint32_t add_user_page(void)
 *	INPUTS: None
 *	OUTPUTS: page_num to load user code into
 *	SIDE EFFECTS: Changes user_pages bitmap and current page_base_addr in the PD, flushes TLB
 */
uint32_t add_user_page(void) {
  uint32_t i, user_bitmap;

  user_bitmap = user_pages;
  for(i = 0; (user_bitmap & 0x01); ++i) user_bitmap = user_bitmap >> 1; // Find the first free page
  user_pages |= (0x01 << i); // Mark page as in use

	set_user_page(i); // Offset to not infringe on kmem
	return i;
}

/* void free_user_page(uint32_t page_num)
 *	INPUTS: page_num - page number to free
 *	OUTPUTS: None
 *	SIDE EFFECTS: Changes user_pages bitmap
 */
void free_user_page(uint32_t page_num) {
  user_pages ^= (0x01 << page_num); // Mark page as not in use
}

/* void set_user_page
 *	INPUTS: uint32_t page_num
 *	OUTPUTS: None
 *	SIDE EFFECTS: Changes page_base_addr of PD at the user memory entry, flushes TLB
 */
void set_user_page(uint32_t page_num) {
  page_directory[USER_MEM_PAGE_INDEX].page_base_addr = page_num; // Update page_base_addr
  flush_tlb();
}

// TODO  make vidmap enable/disable per process

/* uint32_t enable_vidmap(void)
 *	INPUTS: None
 *	OUTPUTS: Virtual address of vidmap memory on success
 *	SIDE EFFECTS: Sets the present bit for Vidmap memory, flushes TLB if not previously set
 */
uint32_t enable_vidmap(void) {
	if (num_vidmaps == 0) {
	//if(!page_table_vidmap[VIDMAP_PAGE_TABLE_INDEX].present) {
		page_table_vidmap[VIDMAP_PAGE_TABLE_INDEX].present = 1;
		flush_tlb();
	}
	num_vidmaps += 1;
	return VIDMAP_MEM_ADDR;
}

/* uint32_t disable_vidmap(void)
 *	INPUTS: None
 *	OUTPUTS: Virtual address of vidmap memory on success
 *	SIDE EFFECTS: Clears the present bit for Vidmap memory, flushes TLB if not previously set
 */
uint32_t disable_vidmap(void) {
	num_vidmaps -= 1;
	//if(page_table_vidmap[VIDMAP_PAGE_TABLE_INDEX].present) {
	if (num_vidmaps == 0) {
		page_table_vidmap[VIDMAP_PAGE_TABLE_INDEX].present = 0;
		flush_tlb();
	}
	return VIDMAP_MEM_ADDR;
}

/* 
 * void map_video_to_video(void)
 *  INPUTS: none
 *  OUTPUTS: none
 *  SIDE EFFECTS: maps video memory to real video memory
 */
void map_video_to_video(void) {
	page_table0[VIDEO_MEM_PAGE_INDEX].page_base_addr = VIDEO_MEM_PAGE_INDEX;
	flush_tlb();
}

/* 
 * void map_vidmap_to_video(void)
 *  INPUTS: none
 *  OUTPUTS: none
 *  SIDE EFFECTS: maps video memory to real video memory
 */
void map_vidmap_to_video(void) {
	page_table_vidmap[VIDMAP_PAGE_TABLE_INDEX].page_base_addr = VIDEO_MEM_PAGE_INDEX;
	flush_tlb();
}

/* 
 * void map_video_to_backup(int n)
 *  INPUTS: n -- backup to map
 *  OUTPUTS: none
 *  SIDE EFFECTS: maps video memory to real video memory
 */
void map_video_to_backup(int n) {
	page_table0[VIDEO_MEM_PAGE_INDEX].page_base_addr = 0x2000+n;
	flush_tlb();
}

/* 
 * void map_vidmap_to_backup(int n)
 *  INPUTS: n -- backup to map
 *  OUTPUTS: none
 *  SIDE EFFECTS: maps video memory to real video memory
 */
void map_vidmap_to_backup(int n) {
	page_table_vidmap[VIDMAP_PAGE_TABLE_INDEX].page_base_addr = 0x2000+n;
	flush_tlb();
}
