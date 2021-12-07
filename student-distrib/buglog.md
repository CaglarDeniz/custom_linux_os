
### `debug.sh` does not exist
Fixed by running `dos2unix` on `debug.sh`  

### Writing cr0 causes program to crash
~~Fixed by doing a jump like the documentation (Intel Arch. Soft. Dev.'s Man.) says.~~  
~~Need to init paging first, since when you turn on paging, you need the jump following to point to already paged memory that has the same linear addr as physical addr, in our case, kernel memory.~~  
I (Justin) changed kernel memory base address from 1 to 4, and that was wrong and causing everything to crash. It's supposed to be 1.  
1 means 1st 4 MB block (0 indexed).  
I (Justin) feel incredibly stupid since it was right before and I spent many hours trying to figure out what was wrong.  

### Video memory was not being paged
Justin was being very stupid and forgot to link the address of the page table to the page directory entry,  
and also forgot that structs are passed by value not by reference.  

### Video memory paged into the wrong place
Justin is very bad at counting bits and remember when stuff needs to be shifted or not.  
Specifically, the base address for the Video memory page was not being right shifted,  
and then Justin forgot to left shift the base address back when converting between virtual and physical addresses.  

### Infinite loop in paging test
Justin doesn't know how to count bits, and thought 4 kB was 10 bits and not 12 bits.  
An overflow caused the loop index to always be less than the `LAST_FOUR_KB_PAGE`,   
and this is because the loop index looped for every 4 kB page in the 4 GB memory.   
The number 4 GB requires a 33rd bit to represent, meaning adding 4 kB repeatedly will result in it overflowing to 0.  
This is why he added `LAST_FOUR_KB_PAGE`, so the loop would stop before it could overflow, but he didn't count his bits right.  

### Not loading IDT after initialization
After I had correctly initialized the IDT values, I found that I was bootlooping when running the IDT test.  
After working with a TA to look through my code, we couldn't find the issue. I later realized I had never called lidt in boot.S

### Using wrong EOI value
When initializing the PIC, interrupts were firing once, and then not again.  
I eventually found that this was because the EOI value declared in the header value was set wrong.  
I'm not sure if I acidentally changed this at some point, or how it happened, but it wasn't a difficult fix once I tracked the issue.

### Not reading data from interrupts
After fixing the EOI value, I was still having a problem where the RTC interrupt only fired once.  
I found out after searching on OSdev that this was because in order to get the RTC to fire again, I needed to read its output value, even if I didn't have any use for it.  

### Trying to initialize a linked list without malloc
Since we want to chain interrupts together, I originally thought the best way to do this would be with a linked list, but I couldn't get it to work.  
I realized that it was because without some memory allocation function, I could only 'create' new things on the stack, which would be later overwritten, so I ended up using an array instead.  

### Failing to print memory contents
When accessing the memory contents we incorrectly initiated `data_base` (base address of data blocks) by adding `base_addr` (base address of filesystem) and `inode_base` (base address of inodes).  
This caused `data_base` to be initialized at the wrong place.  
When accessing data blocks, we incorrectly wrote `(data_block_t*)(inode_base + root.num_inodes * BLOCK_SIZE)` instead of `(data_block_t*)(inode_base + root.num_inodes)` for initializing `data_base`.  
This caused it to be initialized at `root.num_inodes * BLOCK_SIZE` blocks after where the inodes started, instead of `root.num_inodes` blocks.  
This was due to a misunderstanding that `inode_base` was an `inode_t*`, and that adding `1` to a pointer causes it to be incremented by the size of the type it is a pointer of.  

### Incorrect behavior after terminal read
If you tried to read from the terminal, but the input is longer than the read, it's supposed to shift the buffer left, but that wasn't working, which also caused the issue that if there was a newline somewhere in the buffer, it would always immediately return from a read. The fix was that in multiple places, the code was checking against the maximum buffer size, instead of the current buffer size.

### File read page fault
File read was page-faulting because it was adding the full block size instead of the remaining block size in multiple places.

### Keyboard interrupts prints extra space 
I originally fed every scancode received by the interrupt handler into our ASCII-transformation array which resulted in extra spaces
being printed on the screen. We later realized that these extra screens corresponded to the release scan codes and changed the implementation of our keyboard_handler such that it only prints on key presses.

### Page fault with RTC
We were having page faults when enabling the RTC. This was very difficult to fix, and we're still not 100% sure what the actual cause is, but a combination of things managed to fix it. We slowed the RTC to 1024, the maximum required OS frequency, and that solved a lot of problems. We were running the RTC at the maximum frequency and that was too fast for the OS to handle. Also, I was forgetting to mask the PIC interrupt when uninstalling handlers. There was also an issue where passing in the fd was accidentally passing the address of the fd instead, so we were trying to access out of bounds memory. We ended up having to rewrite a lot of the PIC code to get the RTC to play nicely with virutalization.

### Terminal Read Weird Behaviour on Multiple Inputs
Turns out the terminal wasn't clearing the end of the buffer on read, so longer inputs made some weird behaviour happen. We didn't notice until finalizing our tests, but it was easy enough to fix by clearing it.

### get args not getting args
Our problem here was that we were trying to copy something on the stack, but after we had replaced the stack context with the new one for program execution. We fixed this by saving the arguments in our new PCB before changing the stack context.

### ESP0 not being set
Our group didn't really understand the purpose of ESP0, so when trying to implement halt, we were getting a weird error where it would return to where the program was called from and then pagefault (since everything else was being properly set up). We fixed this by learning that esp0 is the kernel stack pointer.

### Assembly linkage clobbering syscall return value 
We were using pusha and popa to save and restore registers and flags before and after a user requested syscall, which unintentionally clobbered the syscall return value in eax. We fixed this by temporarily storing eax on the stack and moving it back after the call to popa

### Page fault on shell exit
With multiple terminals, if you tried to exit one of them, when the scheduler got back to it, it would page fault. It turned out that the original kernel stack was being overwritten, so iret was being given wrong values

### Glitchy pingpong
Neither `halt` nor `switch_process` was setting `fd_table` after the PCB was switched, which meant starting another process caused `rtc_read` to use the wrong values.


We were using pusha and popa to save and restore registers and flags before and after a user requested syscall, which unintentionally clobbered the syscall return value in eax. We fixed this by temporarily storing eax on the stack and moving it back after the call to popa

### getargs() and execute() didn't ignore leading and in-between spaces
This was something that was brought to our attention in discussion, and was a simple fix to skip extra spaces.

### RTC handler file descriptor overloading
The additional handlers registered on the RTC to implement virtualization were modifying file descriptors, and because changing processes changed file descriptors, they were modifying file descriptors they shouldn't be modifying.  
This was fixed by implementing virtualization using a single RTC handler and modifying open RTC file descriptors only if they were currently present in the file descriptor table.  

### Vidmap User memory checking physical addresses instead of virtual
The intial incorrect implementation of vidmap() was checking for whether or not the provided double pointer was in the user's physical memory when it should have been checking if it was in the user's virtual memory address range of 128-132 MB.

### Sigtest causing pagefaults
Specifically there were multiple pagefaults caused by sigtest, and running it also caused trying to exit the shell to page fault multiple times. The issue was that most of the assembly linkage had been updated for scheduling, but not for exceptions.

