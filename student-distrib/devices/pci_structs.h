#ifndef PCI_STRUCTS_H
#define PCI_STRUCTS_H

#include "../lib.h"

typedef union pci_config_address {
  uint32_t value;
  struct {
    uint32_t offset     : 8;
    uint32_t function   : 3;
    uint32_t dev_number : 5;
    uint32_t bus_number : 8;
    uint32_t reserved   : 7;
    uint32_t enable     : 1;
  } __attribute__((packed));
} pci_addr_t;

typedef union pci_base_address_register {
  uint32_t value;
  struct {
    uint32_t is_port      : 1;
    uint32_t type         : 2;
    uint32_t prefetchable : 1;
    uint32_t addr_mmio    : 28;
  };
  struct {
    uint32_t is_port_     : 1;
    uint32_t reserved     : 1;
    uint32_t addr_port    : 30;
  };
} pci_bar_t;

typedef struct pci_device {
  uint32_t bus;
  uint32_t slot;
  uint32_t func;
  union {
    uint32_t regs[16];
    struct {
      uint32_t vendor_id : 16;
      uint32_t device_id : 16;
      uint32_t command  : 16;
      uint32_t status   : 16;
      uint32_t rev_id     : 8;
      uint32_t class_code : 24;
      uint32_t cache_line_size : 8;
      uint32_t lat_timer       : 8;
      uint32_t header_typ      : 8;
      uint32_t bist            : 8;
      pci_bar_t bar[6];
      uint32_t cardbus_cis_pointer;
      uint32_t subsystem_vendor_id : 16;
      uint32_t subsystem_id        : 16;
      uint32_t expansion_rom_base_address;
      uint32_t cap_pointer : 8;
      uint32_t reserved    : 24;
      uint32_t reversed1;
      uint32_t int_line : 8;
      uint32_t int_pin  : 8;
      uint32_t min_gnt  : 8;
      uint32_t max_lat  : 8;
    } __attribute__((packed));
  };
  uint32_t bar_addr[6];
} pci_device_t;

#endif /* PCI_STRUCTS_H */
