#include "e1000.h"
#include "pci.h"
#include "../networking/networking.h"
#include "../paging.h"
#include "../i8259.h"
#include "../lib.h"

static pci_device_t device;

#define VENDOR_ID           0x8086
#define DEVICE_ID           0x100E

#define ENABLE_BUS_MASTERING 4

#define REG_CTRL            0x00000
#define REG_EECD            0x00010
#define REG_EERD            0x00014
#define REG_ICR             0x000C0
// Suggested ITR range 651-5580 (ranging from 6000-700 interrupts/second)
#define REG_ITR             0x000C4
#define REG_IMS             0x000D0
#define REG_IMC             0x000D8
#define REG_RCTRL           0x00100
// RDBA must be 16-byte aligned
#define REG_RDBAL           0x02800
#define REG_RDBAH           0x02804
// RDLEN must be 128-byte aligned (number of descriptors is RDLEN/16)
#define REG_RDLEN           0x02808
#define REG_RDHEAD          0x02810
#define REG_RDTAIL          0x02818
#define REG_TCTL            0x00400
#define REG_TDBAL           0x03800
// TDBA must be 16-byte aligned
#define REG_TDBAL           0x03800
#define REG_TDBAH           0x03804
// TDLEN must be 128-byte aligned (number of descriptors is TDLEN/16)
#define REG_TDLEN           0x03808
#define REG_TDHEAD          0x03810
#define REG_TDTAIL          0x03818
#define REG_RXCSUM          0x05000
#define REG_RAL             0x05400
#define REG_RAH             0x05404
#define REG_MTA             0x05200

#define CTRL_RST            0x4000000
#define CTRL_SLU            0x40

#define EECD_PRES           0x100

#define EERD_START          0x1
#define EERD_DONE           0x10
#define EERD_ADDR_OFFSET    8
#define EERD_DATA_OFFSET    16

#define INT_TXDW            0x01
#define INT_TXQE            0x02
#define INT_LSC             0x04
#define INT_RXSEQ           0x08
#define INT_RXDMT0          0x10
#define INT_RXO             0x40
#define INT_RXT0            0x80

#define RCTRL_EN            0x02
#define RCTRL_SBP           0x04
#define RCTRL_UPE           0x08
#define RCTRL_MPE           0x10
#define RCTRL_LPE           0x20
#define RCTRL_BAM           0x8000
#define RCTRL_SECRC         0x4000000

#define TCTRL_EN            0x02
#define TCTRL_PSP           0x08

#define RXCSUM_IPOFLD       0x100
#define RXCSUM_TUOFLD       0x200

#define RAH_AV              0x80000000

#define MAC_12              0x00
#define MAC_34              0x01
#define MAC_56              0x02

#define NUM_DESCS           8

typedef struct __attribute__((packed)) rx_desc {
    uint32_t addr_low;
    uint32_t addr_high; // zero for us
    uint16_t length;
    uint16_t checksum;
    union {
        struct __attribute__((packed)) {
            uint32_t status_DD      : 1;
            uint32_t status_EOP     : 1;
            uint32_t status_IXSM    : 1;
            uint32_t status_VP      : 1;
            uint32_t status_RSV     : 1;
            uint32_t status_TCPCS   : 1;
            uint32_t status_IPCS    : 1;
            uint32_t status_PIF     : 1;
        };
        uint8_t status;
    };
    uint8_t errors;
    uint16_t special;
} rx_desc_t;

typedef struct __attribute__((packed)) tx_desc {
    uint32_t addr_low;
    uint32_t addr_high; // zero for us
    uint16_t length;
    uint8_t cso;
    uint32_t cmd_EOP        : 1;
    uint32_t cmd_IFCS       : 1;
    uint32_t cmd_IC         : 1;
    uint32_t cmd_RS         : 1;
    uint32_t cmd_RSV        : 1;
    uint32_t cmd_DEXT       : 1;
    uint32_t cmd_VLE        : 1;
    uint32_t cmd_IDE        : 1;
    union {
        struct __attribute__((packed)){
            uint32_t status_DD      : 1;
            uint32_t status_EC      : 1;
            uint32_t status_LC      : 1;
            uint32_t status_RSV     : 1;
            uint32_t rsv            : 4;
        };
        struct {
            uint8_t status;
        };
    };
    uint8_t css;
    uint16_t special;
} tx_desc_t;

static rx_desc_t rx_descs[NUM_DESCS] __attribute__((aligned(64)));
static uint32_t rx_counter;
// 8 2KB rx buffers
static uint8_t rx_buffers[NUM_DESCS][2048];

static tx_desc_t tx_descs[NUM_DESCS] __attribute__((aligned(64)));
static uint32_t tx_counter;

// TODO
static void out(uint32_t port, uint32_t data) {
    *(uint32_t*)(device.bar_addr[0] + port) = data;
}

// TODO
static uint32_t in(uint32_t port) {
    return *(uint32_t*)(device.bar_addr[0] + port);
}

// TODO
uint32_t in_long_e1000(uint32_t port) {
    return *(uint32_t*)(device.bar_addr[0] + port);
}

// TODO
static uint16_t read_eeprom(uint8_t addr) {
    out(REG_EERD, (addr << EERD_ADDR_OFFSET) | EERD_START);
    while (!(in(REG_EERD) & EERD_DONE));
    return in(REG_EERD)>>EERD_DATA_OFFSET;
}

// TODO
static void receive_packet(void) {
    uint32_t counter;
    while (1) {
        counter = rx_counter;
        rx_counter = (rx_counter + 1)%NUM_DESCS;
        // (order doesn't really matter, because we only take one interrupt at a time)
        if (rx_descs[counter].status_DD && rx_descs[counter].status_EOP) {
            //printf("Parsing packet\n");
            parse_packet((uint8_t*)rx_descs[counter].addr_low);
            rx_descs[counter].status_DD = 0;
        } else {
            rx_counter = counter;
            return;
        }
        // update RDTAIL
        out(REG_RDTAIL, counter);
    }
}

// TODO
static void e1000_interrupt(void) {
    uint32_t status;
    status = in(REG_ICR);
    if (status & INT_RXT0) {
        receive_packet();
    }
    if (status & INT_RXO) {
        printf("ICR: 0x%x\n", status);
        die("Rx overflow!\n");
    }
    if (status & INT_RXSEQ) {
        printf("ICR: 0x%x\n", status);
        die("Rx error!\n");
    }
}

// TODO
int e1000_send_packet(uint8_t* packet, uint32_t size) {
  if (size > 1518) return -1; // maximum ethernet II frame size
  // the e1000 does support TCP segmentation, but we don't care

  uint32_t flags;

  cli_and_save(flags);

  uint32_t old_counter = tx_counter;

  tx_descs[tx_counter].addr_low = (uint32_t)packet;
  tx_descs[tx_counter].length = size;
  tx_descs[tx_counter].cmd_EOP = 1; // end of packet
  tx_descs[tx_counter].cmd_IFCS = 1; // add FCS
  tx_descs[tx_counter].cmd_RS = 1; // record status?
  tx_counter = (tx_counter + 1) % NUM_DESCS;
  out(REG_TDTAIL, tx_counter);

  restore_flags(flags);

  while (!tx_descs[old_counter].status); // wait for any status

  return 0;
}

// TODO
int e1000_init(void) {
    int i;

    // Find PCI device
    if (find_device(&device, VENDOR_ID, DEVICE_ID)) return -1;

    // Device bar0 should be MMIO
    if (device.bar[0].is_port) return -1;

    // Enable bus mastering
    device.command |= ENABLE_BUS_MASTERING;
    pci_update(&device, PCI_COMMAND_OFFSET);

    // Add page
    brute_add_page(device.bar_addr[0]);
    brute_add_page(device.bar_addr[0]+6*FOUR_KB);

    // Reset e1000
    out(REG_CTRL, CTRL_RST);
    printf("Reset e1000\n"); // wait (lol)
    while (in(REG_CTRL) & CTRL_RST);

    // Set link up3
    out(REG_CTRL, in(REG_CTRL) | CTRL_SLU);

    // EEPROM should be present
    if (!(in(REG_EECD) & EECD_PRES)) return -1;

    // Read MAC address
    *(uint16_t*)(&our_mac.v[0]) = read_eeprom(0x00);
    *(uint16_t*)(&our_mac.v[2]) = read_eeprom(0x01);
    *(uint16_t*)(&our_mac.v[4]) = read_eeprom(0x02);

    // Enable all except transmit interrupts
    out(REG_IMS, 0x1F6DC); // TODO magic
    // Clear interrupts
    in(REG_ICR);
    // Register interrupt handler
    register_interrupt_handler(device.int_line, e1000_interrupt);

    // MTA
    // TODO magic
    for (i = 0; i < 128; i++) {
        out(REG_MTA + (4*i), 0);
    }

    // Setup Rx descriptors
    for (i = 0; i < NUM_DESCS; i++) {
        rx_descs[i].addr_low = (uint32_t)rx_buffers[i];
        rx_descs[i].status = 0;
    }

    // Point device to Rx descriptors
    out(REG_RDBAH, 0);
    out(REG_RDBAL, (uint32_t)rx_descs);
    out(REG_RDLEN, 128);// TODO magic

    // Point head and tail
    out(REG_RDHEAD, 0);
    out(REG_RDTAIL, NUM_DESCS);
    rx_counter = 0;

    // Enable recieving
    out(REG_RCTRL, RCTRL_EN | RCTRL_SBP | RCTRL_UPE | RCTRL_MPE | RCTRL_BAM | RCTRL_SECRC);

    // Initialize Tx
    out(REG_TDBAH, 0);
    out(REG_TDBAL, (uint32_t)tx_descs);
    out(REG_TDLEN, 128);// TODO magic

    // Point head and tail
    out(REG_TDHEAD, 0);
    out(REG_TDTAIL, NUM_DESCS);
    tx_counter = 0;

    // Enable transmitting (and padding)
    out(REG_TCTL, TCTRL_EN | TCTRL_PSP);

    //printf("status: %x\n", in(REG_TCTL));

    return 0;
}
