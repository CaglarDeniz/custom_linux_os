#ifndef PCI_H
#define PCI_H

#include "../lib.h"
#include "pci_structs.h"

#define PCI_CFG_ADDR_PORT 0xCF8
#define PCI_CFG_DATA_PORT 0xCFC

#define PCI_DEVICE_ID_OFFSET 0x0
#define PCI_VENDOR_ID_OFFSET 0x2
#define PCI_STATUS_OFFSET 0x4
#define PCI_COMMAND_OFFSET 0x6
#define PCI_CLASS_OFFSET 0x8
#define PCI_SUBCLASS_OFFSET 0x9
#define PCI_PROG_IF_OFFSET 0xA
#define PCI_REVISION_ID_OFFSET 0xB
#define PCI_BIST_OFFSET 0xC
#define PCI_HEADER_OFFSET 0xD
#define PCI_LATENCY_OFFSET 0xE
#define PCI_CACHE_LINE_SIZE_OFFSET 0xF

#define PCI_BAR0_REG 0x4
#define PCI_BAR1_REG 0x5
#define PCI_BAR2_REG 0x6
#define PCI_BAR3_REG 0x7
#define PCI_BAR4_REG 0x8
#define PCI_BAR5_REG 0x9

#define PCI_BAR_PORT 0xFFFFFFFC
#define PCI_BAR_MEM  0xFFFFFFF0

#define PCI_INTERRUPT_LINE_OFFSET 0x3F

uint32_t pci_read_long(uint32_t bus, uint32_t slot, uint32_t func, uint32_t reg);
uint16_t pci_read_short(uint32_t bus, uint32_t slot, uint32_t func, uint32_t offset);
uint8_t pci_read_byte(uint32_t bus, uint32_t slot, uint32_t func, uint32_t offset);
int find_device(pci_device_t* dev, uint16_t vendor, uint16_t device);
void pci_write_long(uint32_t bus, uint32_t slot, uint32_t func, uint32_t reg, uint32_t data);
void pci_update(pci_device_t *device, uint32_t offset);

#endif // PCI_H
