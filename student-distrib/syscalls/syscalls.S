# global declarations for syscall table 
.globl halt , execute , read , write , open , close , getargs, vidmap , set_handler , sigreturn , creat, unlink, syscall_handler
.globl syscall_shim

# 
#	Syscall Handler
#
#	Inputs:  int32_t fd , const void*buf , int32_t nbytes *
# 			 syscall number stored in eax, arguments stored in ebx-edx 
#	Outputs: return value of syscall 
#	Side Effects: jumps to the appropriate syscall from the table with the specified arguments
#
syscall_handler: 

pushl $0 # no err_code
pushl $0x80 # syscall (irq 0x80)

jmp do_irq_common


#
# Syscall Shim
#
# Inputs:  int32_t b, int32_t c, int32_t d, int32_t a
#         syscall number in a, arguments in b-d
# Outputs: return value of syscall
# Side Effects: jumps to the appropriate syscall from the table with the specified arguments
#
syscall_shim:

# move a to eax
movl 16(%esp), %eax

# use jumptable (%eax was vetted by whatever called syscall_shim)
jmp *syscall_table(,%eax,4)


# pointer table order in appendix b

# A syscall address table to do assembly linkage
syscall_table:

.long 0x0 # empty table entry to facilitate table access
.long halt # halt
.long execute # execute
.long read
.long write
.long open
.long close
.long getargs # getargs
.long vidmap # vidmap
.long 0x0 # set_handler
.long 0x0 # sigreturn
.long creat
.long unlink








