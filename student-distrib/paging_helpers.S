# paging_helpers.S - Set control registers to allow for paging
# vim:ts=4 noexpandtab

#define ASM
#include "x86_desc.h"

.data
	CR0_ENABLE_PAGING 	= 0x80000000		# paging enable
	CR0_ENABLE_PROTECTED_MODE = 0x00000001	# prevents user from modifying control regs
	CR4_ENABLE_PAGE_SIZE_EXTENSION 	= 0x00000010	# enable 4 MB pages
	CR4_PAGE_GLOBAL_ENABLE = 0x00000080		# enable global pages 


.globl set_control_regs, read_physical_addr, flush_tlb

.text

# void set_control_regs(unsigned long page_directory_ptr);
# Performs three tasks:
#	(1) loads page_directory_ptr into cr3
#	(2) enables page size extension, page global enable in cr4 
#	(3) enables paging and protected mode in cr0
# Inputs   : page_directory_ptr - location of the page_directory in memory
# Outputs  : none
# Registers: Standard C calling convention
set_control_regs:
		pushl 	%ebp
		movl	%esp, %ebp
		
		# Load the page_directory location into cr3
		movl 	8(%ebp), %eax
		movl 	%eax, %cr3

		movl	%cr4, %eax
		# Enable Page Size Extension [4] (allow 4 MB pages)
		orl		$CR4_ENABLE_PAGE_SIZE_EXTENSION, %eax
		# Page Global Enable [7] (allow global pages not invalidated in TLB)
		orl		$CR4_PAGE_GLOBAL_ENABLE, %eax
		movl 	%eax, %cr4

		# Enable paging [31], Protected Mode Enable [0] (Protected mode is necessary for paging) 
		movl 	%cr0, %eax
		orl		$CR0_ENABLE_PAGING, %eax
		 # NOTE: protected mode should already be set (long procedure found in 9.9.1 in IASDM)
		orl		$CR0_ENABLE_PROTECTED_MODE, %eax
		movl 	%eax, %cr0
		
	set_control_regs_ret:
		leave 
		ret

# DEPRECATED: You should just dereference the address if you want to read from it
# uint8_t read_physical_addr(uint32_t physical_addr);
# Reads from memory at physical_addr
# Inputs   : physical_addr - address in physical memory
# Outputs  : byte read from physical memory
# Registers: Standard C calling convention
read_physical_addr:
		pushl 	%ebp
		movl	%esp, %ebp
		
		movl	8(%ebp), %eax
		movl 	(%eax), %eax

	read_physical_addr_ret:
		leave 
		ret

# void flush_tlb(void);
# Flushes TLB
# Inputs   : none
# Outputs  : none
# Registers: Standard C calling convention
flush_tlb:
		pushl 	%ebp
		movl	%esp, %ebp 
		
		movl	%cr3,%eax
		movl	%eax,%cr3
		
	flush_tlb_ret:
		leave 
		ret
