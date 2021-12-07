
# global declarations for IRQ assembly linkers 
.globl do_irq_0,do_irq_1,do_irq_2,do_irq_3,do_irq_4,do_irq_5,do_irq_6,do_irq_7,do_irq_8,do_irq_9,do_irq_10,do_irq_11,do_irq_12,do_irq_13,do_irq_14,do_irq_15
.globl do_exc_0, do_exc_1, do_exc_2, do_exc_3, do_exc_4, do_exc_5, do_exc_6, do_exc_7, do_exc_8, do_exc_9, do_exc_10, do_exc_11, do_exc_12, do_exc_13, do_exc_14, do_exc_15, do_exc_16, do_exc_17, do_exc_18, do_exc_19, do_exc_20, do_exc_21


.globl do_irq_main, swap_context
.globl do_irq_common

#
# Provides general assembly linkage for IRQs
# Inputs : None
# Outputs: None
# Side Effects : Calls related handlers for IRQ
#
do_irq_common:

# finish setting up hw_context
pushl %ebp
pushl %edi
pushl %esi
pushl %edx
pushl %ecx
pushl %ebx
pushl %eax
pushl $0

# swap context
pushl %esp
call do_irq_main

# set esp and swap
movl %eax, %esp
call swap_context
addl $4, %esp

# pop context
popl %eax
popl %ebx
popl %ecx
popl %edx
popl %esi
popl %edi
popl %ebp

# pop error_code and irq_num
addl $8, %esp

# iret
iret


#
# Provides assembly linkage for IRQ 0
# Inputs : None 
# Outputs: None 
# Side Effects : Calls related handlers for IRQ 0 
#
do_irq_0:

pushl $0 # no err_code
pushl $0+32 # irq 0

jmp do_irq_common

#
# Provides assembly linkage for IRQ 1
# Inputs : None 
# Outputs: None 
# Side Effects : Calls related handlers for IRQ 1 
#
do_irq_1:

pushl $0 # no err_code
pushl $1+32 # irq 1

jmp do_irq_common

#
# Provides assembly linkage for IRQ 2
# Inputs : None 
# Outputs: None 
# Side Effects : Calls related handlers for IRQ 2 
#
do_irq_2:

pushl $0 # no err_code
pushl $2+32 # irq 2

jmp do_irq_common

#
# Provides assembly linkage for IRQ 3
# Inputs : None 
# Outputs: None 
# Side Effects : Calls related handlers for IRQ 3 
#
do_irq_3:

pushl $0 # no err_code
pushl $3+32 # irq 3

jmp do_irq_common

#
# Provides assembly linkage for IRQ 4
# Inputs : None 
# Outputs: None 
# Side Effects : Calls related handlers for IRQ 4 
#
do_irq_4:

pushl $0 # no err_code
pushl $4+32 # irq 4

jmp do_irq_common

#
# Provides assembly linkage for IRQ 5
# Inputs : None 
# Outputs: None 
# Side Effects : Calls related handlers for IRQ 5 
#
do_irq_5:

pushl $0 # no err_code
pushl $5+32 # irq 5

jmp do_irq_common

#
# Provides assembly linkage for IRQ 6
# Inputs : None 
# Outputs: None 
# Side Effects : Calls related handlers for IRQ 6 
#
do_irq_6:

pushl $0 # no err_code
pushl $6+32 # irq 6

jmp do_irq_common

#
# Provides assembly linkage for IRQ 7
# Inputs : None 
# Outputs: None 
# Side Effects : Calls related handlers for IRQ 7 
#
do_irq_7:

pushl $0 # no err_code
pushl $7+32 # irq 7

jmp do_irq_common

#
# Provides assembly linkage for IRQ 8
# Inputs : None 
# Outputs: None 
# Side Effects : Calls related handlers for IRQ 8 
#
do_irq_8:

pushl $0 # no err_code
pushl $8+32 # irq 8

jmp do_irq_common

#
# Provides assembly linkage for IRQ 9
# Inputs : None 
# Outputs: None 
# Side Effects : Calls related handlers for IRQ 9 
#
do_irq_9:

pushl $0 # no err_code
pushl $9+32 # irq 9

jmp do_irq_common

#
# Provides assembly linkage for IRQ 10
# Inputs : None 
# Outputs: None 
# Side Effects : Calls related handlers for IRQ 10 
#
do_irq_10:

pushl $0 # no err_code
pushl $10+32 # irq 10

jmp do_irq_common

#
# Provides assembly linkage for IRQ 11
# Inputs : None 
# Outputs: None 
# Side Effects : Calls related handlers for IRQ 11 
#
do_irq_11:

pushl $0 # no err_code
pushl $11+32 # irq 11

jmp do_irq_common

#
# Provides assembly linkage for IRQ 12
# Inputs : None 
# Outputs: None 
# Side Effects : Calls related handlers for IRQ 12 
#
do_irq_12:

pushl $0 # no err_code
pushl $12+32 # irq 12

jmp do_irq_common

#
# Provides assembly linkage for IRQ 13
# Inputs : None 
# Outputs: None 
# Side Effects : Calls related handlers for IRQ 13 
#
do_irq_13:

pushl $0 # no err_code
pushl $13+32 # irq 13

jmp do_irq_common

#
# Provides assembly linkage for IRQ 14
# Inputs : None 
# Outputs: None 
# Side Effects : Calls related handlers for IRQ 14 
#
do_irq_14:

pushl $0 # no err_code
pushl $14+32 # irq 14

jmp do_irq_common

#
# Provides assembly linkage for IRQ 15
# Inputs : None 
# Outputs: None 
# Side Effects : Calls related handlers for IRQ 15 
#
do_irq_15:

pushl $0 # no err_code
pushl $15+32 # irq 15

jmp do_irq_common

#
# Provides assembly linkage for exception 0 
# Inputs : None
# Outputs: None
# Side Effects : Calls related handlers for exception 0
#
do_exc_0:

pushl $0 # no err_code
pushl $0 # exception 0

jmp do_irq_common

#
# Provides assembly linkage for exception 1 
# Inputs : None
# Outputs: None
# Side Effects : Calls related handlers for exception 1
#
do_exc_1:

pushl $0 # no err_code
pushl $1 # exception 1

jmp do_irq_common


#
# Provides assembly linkage for exception 2 
# Inputs : None
# Outputs: None
# Side Effects : Calls related handlers for exception 2
#
do_exc_2:

pushl $0 # no err_code
pushl $2 # exception 2

jmp do_irq_common


#
# Provides assembly linkage for exception 3 
# Inputs : None
# Outputs: None
# Side Effects : Calls related handlers for exception 3
#
do_exc_3:

pushl $0 # no err_code
pushl $3 # exception 3

jmp do_irq_common


#
# Provides assembly linkage for exception 4 
# Inputs : None
# Outputs: None
# Side Effects : Calls related handlers for exception 4
#
do_exc_4:

pushl $0 # no err_code
pushl $4 # exception 4

jmp do_irq_common


#
# Provides assembly linkage for exception 5 
# Inputs : None
# Outputs: None
# Side Effects : Calls related handlers for exception 5
#
do_exc_5:

pushl $0 # no err_code
pushl $5 # exception 5

jmp do_irq_common


#
# Provides assembly linkage for exception 6 
# Inputs : None
# Outputs: None
# Side Effects : Calls related handlers for exception 6
#
do_exc_6:

pushl $0 # no err_code
pushl $6 # exception 6

jmp do_irq_common


#
# Provides assembly linkage for exception 7 
# Inputs : None
# Outputs: None
# Side Effects : Calls related handlers for exception 7
#
do_exc_7:

pushl $0 # no err_code
pushl $7 # exception 7

jmp do_irq_common


#
# Provides assembly linkage for exception 8 
# Inputs : None
# Outputs: None
# Side Effects : Calls related handlers for exception 8
#
do_exc_8:

pushl $8 # exception 8

jmp do_irq_common


#
# Provides assembly linkage for exception 9 
# Inputs : None
# Outputs: None
# Side Effects : Calls related handlers for exception 9
#
do_exc_9:

pushl $0 # no err_code
pushl $9 # exception 9

jmp do_irq_common


#
# Provides assembly linkage for exception 10 
# Inputs : None
# Outputs: None
# Side Effects : Calls related handlers for exception 10
#
do_exc_10:

pushl $10 # exception 10

jmp do_irq_common


#
# Provides assembly linkage for exception 11 
# Inputs : None
# Outputs: None
# Side Effects : Calls related handlers for exception 11
#
do_exc_11:

pushl $11 # exception 11

jmp do_irq_common


#
# Provides assembly linkage for exception 12 
# Inputs : None
# Outputs: None
# Side Effects : Calls related handlers for exception 12
#
do_exc_12:

pushl $12 # exception 12

jmp do_irq_common


#
# Provides assembly linkage for exception 13 
# Inputs : None
# Outputs: None
# Side Effects : Calls related handlers for exception 13
#
do_exc_13:

pushl $13 # exception 13

jmp do_irq_common


#
# Provides assembly linkage for exception 14 
# Inputs : None
# Outputs: None
# Side Effects : Calls related handlers for exception 14
#
do_exc_14:

pushl $14 # exception 14

jmp do_irq_common


#
# Provides assembly linkage for exception 15 
# Inputs : None
# Outputs: None
# Side Effects : Calls related handlers for exception 15
#
do_exc_15:

pushl $0 # no err_code # ?
pushl $15 # exception 15

jmp do_irq_common



#
# Provides assembly linkage for exception 16 
# Inputs : None
# Outputs: None
# Side Effects : Calls related handlers for exception 16
#
do_exc_16:

pushl $0 # no err_code
pushl $16 # exception 16

jmp do_irq_common


#
# Provides assembly linkage for exception 17 
# Inputs : None
# Outputs: None
# Side Effects : Calls related handlers for exception 17
#
do_exc_17:

pushl $17 # exception 17

jmp do_irq_common


#
# Provides assembly linkage for exception 18 
# Inputs : None
# Outputs: None
# Side Effects : Calls related handlers for exception 18
#
do_exc_18:

pushl $0 # no err_code
pushl $18 # exception 18

jmp do_irq_common


#
# Provides assembly linkage for exception 19 
# Inputs : None
# Outputs: None
# Side Effects : Calls related handlers for exception 19
#
do_exc_19:

pushl $0 # no err_code
pushl $19 # exception 19

jmp do_irq_common


#
# Provides assembly linkage for exception 20 
# Inputs : None
# Outputs: None
# Side Effects : Calls related handlers for exception 20
#
do_exc_20:

pushl $0 # no err_code
pushl $20 # exception 20

jmp do_irq_common


#
# Provides assembly linkage for exception 21 
# Inputs : None
# Outputs: None
# Side Effects : Calls related handlers for exception 21
#
do_exc_21:

pushl $1 # no err_code
pushl $21 # exception 21

jmp do_irq_common

