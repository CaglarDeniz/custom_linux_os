/* kernel.c - the C part of the kernel
 * vim:ts=4 noexpandtab
 */

#include "multiboot.h"
#include "x86_desc.h"
#include "lib.h"
#include "i8259.h"
#include "debug.h"
#include "tests.h"
#include "tty.h"
#include "paging.h"
#include "idt.h"
#include "devices/devices.h"
#include "filesystem/filesystem.h"
#include "syscalls/syscalls.h"
#include "loader.h"
#include "tasks/scheduling.h"
#include "devices/pci.h"
#include "networking/networking.h"
#include "devices/e1000.h"
#include "networking/http.h"

#define RUN_TESTS

/* Macros. */
/* Check if the bit BIT in FLAGS is set. */
#define CHECK_FLAG(flags, bit)   ((flags) & (1 << (bit)))

static void test_rtc(uint32_t n) {
    printf("LOL! %d\n", n);
}

/* Check if MAGIC is valid and print the Multiboot information structure
   pointed by ADDR. */
void entry(unsigned long magic, unsigned long addr) {

    multiboot_info_t *mbi;

    /* Init the terminal */
    tty_init();

    /* Clear the screen. */
    clear();

    /* Am I booted by a Multiboot-compliant boot loader? */
    if (magic != MULTIBOOT_BOOTLOADER_MAGIC) {
        printf("Invalid magic number: 0x%#x\n", (unsigned)magic);
        return;
    }

    /* Set MBI to the address of the Multiboot information structure. */
    mbi = (multiboot_info_t *) addr;

    /* Print out the flags. */
    printf("flags = 0x%#x\n", (unsigned)mbi->flags);

    /* Are mem_* valid? */
    if (CHECK_FLAG(mbi->flags, 0))
        printf("mem_lower = %uKB, mem_upper = %uKB\n", (unsigned)mbi->mem_lower, (unsigned)mbi->mem_upper);

    /* Is boot_device valid? */
    if (CHECK_FLAG(mbi->flags, 1))
        printf("boot_device = 0x%#x\n", (unsigned)mbi->boot_device);

    /* Is the command line passed? */
    if (CHECK_FLAG(mbi->flags, 2))
        printf("cmdline = %s\n", (char *)mbi->cmdline);

    if (CHECK_FLAG(mbi->flags, 3)) {
        int mod_count = 0;
        int i;
        module_t* mod = (module_t*)mbi->mods_addr;
        while (mod_count < mbi->mods_count) {
            printf("Module %d loaded at address: 0x%#x\n", mod_count, (unsigned int)mod->mod_start);
			filesystem_init((unsigned int)mod->mod_start);
            printf("Module %d ends at address: 0x%#x\n", mod_count, (unsigned int)mod->mod_end);
            printf("First few bytes of module:\n");
            for (i = 0; i < 16; i++) {
                printf("0x%x ", *((char*)(mod->mod_start+i)));
            }
            printf("\n");
            mod_count++;
            mod++;
        }
    }
    /* Bits 4 and 5 are mutually exclusive! */
    if (CHECK_FLAG(mbi->flags, 4) && CHECK_FLAG(mbi->flags, 5)) {
        printf("Both bits 4 and 5 are set.\n");
        return;
    }

    /* Is the section header table of ELF valid? */
    if (CHECK_FLAG(mbi->flags, 5)) {
        elf_section_header_table_t *elf_sec = &(mbi->elf_sec);
        printf("elf_sec: num = %u, size = 0x%#x, addr = 0x%#x, shndx = 0x%#x\n",
                (unsigned)elf_sec->num, (unsigned)elf_sec->size,
                (unsigned)elf_sec->addr, (unsigned)elf_sec->shndx);
    }

    /* Are mmap_* valid? */
    if (CHECK_FLAG(mbi->flags, 6)) {
        memory_map_t *mmap;
        printf("mmap_addr = 0x%#x, mmap_length = 0x%x\n",
                (unsigned)mbi->mmap_addr, (unsigned)mbi->mmap_length);
        for (mmap = (memory_map_t *)mbi->mmap_addr;
                (unsigned long)mmap < mbi->mmap_addr + mbi->mmap_length;
                mmap = (memory_map_t *)((unsigned long)mmap + mmap->size + sizeof (mmap->size)))
            printf("    size = 0x%x, base_addr = 0x%#x%#x\n    type = 0x%x,  length    = 0x%#x%#x\n",
                    (unsigned)mmap->size,
                    (unsigned)mmap->base_addr_high,
                    (unsigned)mmap->base_addr_low,
                    (unsigned)mmap->type,
                    (unsigned)mmap->length_high,
                    (unsigned)mmap->length_low);
    }

    /* Construct an LDT entry in the GDT */
    {
        seg_desc_t the_ldt_desc;
        the_ldt_desc.granularity = 0x0;
        the_ldt_desc.opsize      = 0x1;
        the_ldt_desc.reserved    = 0x0;
        the_ldt_desc.avail       = 0x0;
        the_ldt_desc.present     = 0x1;
        the_ldt_desc.dpl         = 0x0;
        the_ldt_desc.sys         = 0x0;
        the_ldt_desc.type        = 0x2;

        SET_LDT_PARAMS(the_ldt_desc, &ldt, ldt_size);
        ldt_desc_ptr = the_ldt_desc;
        lldt(KERNEL_LDT);
    }

    /* Construct a TSS entry in the GDT */
    {
        seg_desc_t the_tss_desc;
        the_tss_desc.granularity   = 0x0;
        the_tss_desc.opsize        = 0x0;
        the_tss_desc.reserved      = 0x0;
        the_tss_desc.avail         = 0x0;
        the_tss_desc.seg_lim_19_16 = TSS_SIZE & 0x000F0000;
        the_tss_desc.present       = 0x1;
        the_tss_desc.dpl           = 0x0;
        the_tss_desc.sys           = 0x0;
        the_tss_desc.type          = 0x9;
        the_tss_desc.seg_lim_15_00 = TSS_SIZE & 0x0000FFFF;

        SET_TSS_PARAMS(the_tss_desc, &tss, tss_size);

        tss_desc_ptr = the_tss_desc;

        tss.ldt_segment_selector = KERNEL_LDT;
        tss.ss0 = KERNEL_DS;
        tss.esp0 = 0x800000;
        ltr(KERNEL_TSS);
    }

    /* Init paging */
    paging_init();

	/* Init the IDT */
	idt_init();

    /* Init the PIC */
    i8259_init();
	i8259_mask_all();

	/* Init devices */
	keyboard_init();
	rtc_init();

	/* Set up STDIN/STDOUT for kernel */
    setup_fdtable(fd_table);

    /* Initialize devices, memory, filesystem, enable device interrupts on the
     * PIC, any other initialization stuff... */

    init_networking();

    /* Enable interrupts */
    /* Do not enable the following until after you have set up your
     * IDT correctly otherwise QEMU will triple fault and simple close
     * without showing you any output */
    printf("Enabling Interrupts\n");
	sti();

	dhcp_init(); // must be run with interrupts enabled

#ifdef RUN_TESTS
    /* Run tests */
    launch_tests();
#endif

    /*// run "nc -ul 56565" on your machine to test
    ip_t lol = {{10, 0, 2, 2}};//{{10, 194, 248, 187}};//{{10, 195, 198, 131}};//
    uint8_t blah[123];
    int8_t *message = "Type something bro: ";

    udp_send_packet(&lol, 56565, 57575, (uint8_t*)message, strlen(message));
    udp_recv(57575, blah, 123);
    printf("%s\n", blah);*/

    /*ip_t blah;
    uint8_t buffer[123];
    printf("Enter a domain name: ");
    int s = tty_read(0, buffer, 123);
    if (s > 0) {
        buffer[s-1] = '\0'; // /n
        dns_query(buffer, &blah);
        printf("Ip address is: ");
        print_ip(&blah);
    } else {
        printf("Bad\n");
    }*/

    /*uint8_t buf[123];
    int idx = tcp_connect("10.0.2.2", 56565);
    int i;
    char *message = "Type something bro: ";
    if (idx > -1) {
        tcp_sendall(idx, (uint8_t*)message, strlen(message));
        i = tcp_recv(idx, buf, 5);
        if (i > 0) buf[i] = '\0';
        printf("Received '%s'\n", buf);
    } else {
        printf("Couldn't connect\n");
    }*/
    //char* buffer = "chrx.local";//[123];
    //int s = strlen(buffer)+1;
    char domain[123];
    printf("Enter a domain name: ");
    int s = tty_read(0, domain, 123);
    if (s > 1) {
        domain[s-1] = '\0';
        int idx = http_get(domain, "/");
        if (idx == -1) {
            printf("Couldn't connect\n");
        } else {
            char buffer[1024+1];
            while (http_recv(idx, buffer, 1024)) {
                puts(buffer);
            }
        }
        /*
        int idx = tcp_connect(buffer, 80);
        char *message = "GET / HTTP/1.0\r\n\r\n";
        uint8_t buf[2049];
        uint32_t i;
        if (idx > -1) {
            tcp_sendall(idx, (uint8_t*)message, strlen(message));
            do {
                i = tcp_recv(idx, buf, 2048);
                if (i > 0) {
                    buf[i] = '\0';
                    puts((char*)buf);
                }
            } while (i > 0);
        } else {
            printf("Couldn't connect\n");
        }*/
    } else {
        printf("Bad\n");
    }

	/* Run User shell */
	// while(1) execute((uint8_t*) "shell");

    /* registering the scheduling handler */
	cli();
	register_interrupt_handler(RTC_IRQ,round_robin) ;
	sti();

	/* Wait */
	while(1);
}
